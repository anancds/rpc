/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ps/core/scheduler_node.h"

namespace mindspore {
namespace ps {
namespace core {

SchedulerNode::~SchedulerNode() {
  MS_LOG(INFO) << "Stop scheduler node!";
  if (!is_already_stopped_) {
    is_already_stopped_ = true;
    server_->Stop();
    if (scheduler_thread_->joinable()) {
      scheduler_thread_->join();
    }
    if (state_flush_thread_->joinable()) {
      state_flush_thread_->join();
    }
    is_ready_ = true;
  }
}

void SchedulerNode::Start() {
  MS_LOG(INFO) << "Start scheduler node!";
  Init();
  Initialize();
  StartClusterStateFlushTimer();
  WaitForStart();
  MS_LOG(INFO) << "The scheduler is ready!";
}

void SchedulerNode::ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  HeartbeatMessage heartbeat_message;
  heartbeat_message.ParseFromString(message.data());

  node_manager_.UpdateHeartbeat(heartbeat_message.node_id());

  HeartbeatRespMessage heartbeat_resp_message;
  heartbeat_resp_message.set_is_cluster_ready(node_manager_.is_cluster_ready());
  heartbeat_resp_message.set_is_cluster_finish(node_manager_.is_cluster_finish());
  heartbeat_resp_message.set_is_cluster_timeout(node_manager_.is_cluster_timeout());

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message.pb_meta()};
  comm_message.set_data(heartbeat_resp_message.SerializeAsString());
  const_cast<TcpServer &>(server).SendMessage(conn, comm_message);
}

void SchedulerNode::Initialize() {
  is_already_stopped_ = false;
  node_info_.node_id_ = CommUtil::GenerateUUID();
  node_info_.node_role_ = NodeRole::SCHEDULER;
  MS_LOG(INFO) << "The node role is:" << CommUtil::NodeRoleToString(node_info_.node_role_)
               << ", the node id is:" << node_info_.node_id_;
}

void SchedulerNode::Init() {
  node_manager_.InitNodeNum();

  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();
  server_ = std::make_unique<TcpServer>(scheduler_host, scheduler_port);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
    switch (message.pb_meta().cmd()) {
      case NodeCommand::HEARTBEAT:
        ProcessHeartBeat(server, conn, message);
        break;
      case NodeCommand::REGISTER:
        ProcessRegister(server, conn, message);
        break;
      case NodeCommand::FINISH:
        ProcessFinish(server, conn, message);
        break;
      case NodeCommand::FETCH_SERVER:
        ProcessFetchServers(server, conn, message);
        break;
      default:
        MS_LOG(EXCEPTION) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
  });

  server_->Init();

  scheduler_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The scheduler node start a tcp server!";
    server_->Start();
  });
  scheduler_thread_->detach();
}

void SchedulerNode::ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  MS_LOG(INFO) << "The scheduler process a register message!";
  RegisterMessage register_message;
  register_message.ParseFromString(message.data());

  // assign worker node and server node rank id
  int rank_id = node_manager_.NextRankId(register_message);
  if (rank_id < 0) {
    MS_LOG(EXCEPTION) << "The rank id is wrong!";
  }
  const std::string &node_id = register_message.node_id();
  node_manager_.UpdateHeartbeat(node_id);

  RegisterRespMessage register_resp_message;
  register_resp_message.set_node_id(node_id);
  register_resp_message.set_rank_id(rank_id);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message.pb_meta()};
  comm_message.set_data(register_resp_message.SerializeAsString());
  const_cast<TcpServer &>(server).SendMessage(conn, comm_message);
}

void SchedulerNode::ProcessFinish(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  FinishMessage finish_message;
  finish_message.ParseFromString(message.data());
  node_manager_.AddFinishNode(finish_message);
  MS_LOG(INFO) << "Process finish message from node id:" << finish_message.node_id();
  const_cast<TcpServer &>(server).SendMessage(conn, message);
}

void SchedulerNode::ProcessFetchServers(const TcpServer &server, const TcpConnection &conn,
                                        const CommMessage &message) {
  FetchServersRespMessage fetch_servers_message;
  std::vector<ServersMeta> servers_meta_list = node_manager_.FetchServersMeta();

  *fetch_servers_message.mutable_servers_meta() = {servers_meta_list.begin(), servers_meta_list.end()};

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message.pb_meta()};
  comm_message.set_data(fetch_servers_message.SerializeAsString());
  const_cast<TcpServer &>(server).SendMessage(conn, comm_message);
}

void SchedulerNode::StartClusterStateFlushTimer() {
  MS_LOG(WARNING) << "The scheduler start a heartbeat timer!";
  state_flush_thread_ = std::make_unique<std::thread>([&]() {
    auto start_time = std::chrono::steady_clock::now();
    while (!is_finish_.load()) {
      // 1. update cluster timeout
      if (!node_manager_.is_cluster_ready() && (std::chrono::steady_clock::now() - start_time >
                                                std::chrono::seconds(ClusterConfig::cluster_available_timeout()))) {
        node_manager_.CheckClusterTimeout();
      }

      // 2. update cluster state
      std::this_thread::sleep_for(std::chrono::seconds(ClusterConfig::heartbeat_interval()));
      node_manager_.UpdateClusterState();
      if (node_manager_.is_cluster_ready()) {
        is_ready_ = true;
        wait_start_cond_.notify_all();
      }
      if (node_manager_.is_cluster_finish()) {
        std::this_thread::sleep_for(std::chrono::seconds(ClusterConfig::heartbeat_interval() * 2));
        is_finish_ = true;
        wait_finish_cond_.notify_all();
      }
    }
  });
  state_flush_thread_->detach();
}

void SchedulerNode::Stop() {
  MS_LOG(INFO) << "Stop scheduler node!";
  if (!is_already_stopped_) {
    is_already_stopped_ = true;
    server_->Stop();
    if (scheduler_thread_->joinable()) {
      scheduler_thread_->join();
    }
    if (state_flush_thread_->joinable()) {
      state_flush_thread_->join();
    }
    is_ready_ = true;
  }
}

void SchedulerNode::Finish() {
  MS_LOG(INFO) << "Finish scheduler node!";
  std::unique_lock<std::mutex> lock(wait_finish_mutex_);
  wait_finish_cond_.wait(lock, [&] {
    if (is_finish_.load()) {
      MS_LOG(INFO) << "The scheduler finish success!";
    }
    return is_finish_.load();
  });
}

void SchedulerNode::Send(const enum NodeRole &node_role, const uint32_t &rank_id, const std::string &message,
                         const uint32_t &timeout) {
  MS_LOG(EXCEPTION) << "The scheduler node is not supported send data to other nodes!";
}

void SchedulerNode::Send(const NodeRole &node_role, const std::vector<uint32_t> &rank_ids,
                         const std::vector<std::string> &data, const uint32_t &timeout) {
  MS_LOG(EXCEPTION) << "The scheduler node is not supported send data to other nodes!";
}

void SchedulerNode::Send(const enum NodeRole &node_role, const uint32_t &rank_id, const std::string &message,
                         CommMessage *comm_message, const uint32_t &timeout) {
  MS_LOG(EXCEPTION) << "The scheduler node is not supported send data to other nodes!";
}

void SchedulerNode::Send(const NodeRole &node_role, const std::vector<uint32_t> &rank_ids,
                         const std::vector<std::string> &data, std::vector<CommMessage *> *comm_message_resp,
                         const uint32_t &timeout) {
  MS_LOG(EXCEPTION) << "The scheduler node is not supported send data to other nodes!";
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore