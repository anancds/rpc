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
  if (!is_node_stop_) {
    is_node_stop_ = true;
    server_->Stop();
    if (scheduler_thread_->joinable()) {
      scheduler_thread_->join();
    }
    is_cluster_ready_ = true;
  }
}

void SchedulerNode::Start() {
  MS_LOG(INFO) << "Start scheduler node!";
  Init();
  InitNode();
  StartHeartbeatTimer();
  StartClusterAvailableTimer();

  //改成bool，是不需要发送terminal指令
  while (!is_cluster_ready_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  MS_LOG(INFO) << "The scheduler is ready to quit!";
}

void SchedulerNode::ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  HeartbeatMessage heartbeat_message;
  heartbeat_message.ParseFromString(message.data());

  UpdateHeartbeat(heartbeat_message.node_id());

  HeartbeatRespMessage heartbeat_resp_message;
  heartbeat_resp_message.set_is_cluster_ready(is_cluster_ready_);
  heartbeat_resp_message.set_is_cluster_finish(is_cluster_finish_);
  heartbeat_resp_message.set_is_node_timeout(is_node_timeout_);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message.pb_meta()};
  comm_message.set_data(heartbeat_resp_message.SerializeAsString());
  const_cast<TcpServer &>(server).SendMessage(conn, comm_message);
}

void SchedulerNode::InitNode() {
  is_node_stop_ = false;
  node_info_.node_id_ = CommUtil::GenerateUUID();
  node_info_.node_role_ = NodeRole::SCHEDULER;
  MS_LOG(INFO) << "The node role is:" << CommUtil::NodeRoleToString(node_info_.node_role_)
               << ", the node id is:" << node_info_.node_id_;
}

void SchedulerNode::Init() {
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
        MS_LOG(INFO) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
  });

  server_->Init();

  scheduler_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The scheduler node start a tcp server!";
    server_->Start();
  });
  scheduler_thread_->detach();
}

void SchedulerNode::UpdateHeartbeat(const std::string &node_id) {
  std::lock_guard<std::mutex> lock(heartbeat_mutex_);
  if (nodes_info_.find(node_id) == nodes_info_.end()) {
    MS_LOG(INFO) << "The node id:" << node_id << " is not registered!";
  }
  NodeInfo node_info = nodes_info_[node_id];
  struct timeval current_time {};
  (void)gettimeofday(&current_time, nullptr);
  heartbeats_[node_id] = current_time;
  MS_LOG(INFO) << "The node role: " << CommUtil::NodeRoleToString(node_info.node_role_) << ", the node id:" << node_id
               << ", the node rank id:" << node_info.rank_id_ << " the current time is: " << current_time.tv_sec;
}

void SchedulerNode::ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  MS_LOG(INFO) << "The scheduler process a register message!";
  RegisterMessage register_message;
  register_message.ParseFromString(message.data());

  // assign worker node and server node rank id
  int rank_id = AssignRankId(register_message);
  if (rank_id < 0) {
    MS_LOG(EXCEPTION) << "The rank id is wrong!";
  }
  const std::string &node_id = register_message.node_id();

  RegisterRespMessage register_resp_message;
  register_resp_message.set_node_id(node_id);
  register_resp_message.set_rank_id(rank_id);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message.pb_meta()};
  comm_message.set_data(register_resp_message.SerializeAsString());
  const_cast<TcpServer &>(server).SendMessage(conn, comm_message);
}

int SchedulerNode::AssignRankId(const RegisterMessage &register_message) {
  std::lock_guard<std::mutex> lock(assign_rank_id_mutex_);
  int rank_id = -1;

  const std::string &node_id = register_message.node_id();
  if (nodes_info_.find(node_id) != nodes_info_.end()) {
    rank_id = nodes_info_[node_id].rank_id_;
    MS_LOG(INFO) << "The node id: " << node_id << " is already assigned!";
    return rank_id;
  }

  if (register_message.role() == NodeRole::SERVER) {
    const std::string &ip = register_message.ip();
    uint32_t port = register_message.port();

    rank_id = ++current_server_rank_id_;
    NodeInfo node_info;
    node_info.node_role_ = NodeRole::SERVER;
    node_info.node_id_ = node_id;
    node_info.rank_id_ = rank_id;
    node_info.ip_ = ip;
    node_info.port_ = port;
    nodes_info_[node_id] = node_info;
    MS_LOG(INFO) << "The server node id:" << node_id << ",node ip: " << node_info.ip_ << ",node port:" << port
                 << " assign rank id:" << rank_id;

  } else if (register_message.role() == NodeRole::WORKER) {
    rank_id = ++current_worker_rank_id_;
    NodeInfo node_info;
    node_info.node_role_ = NodeRole::SERVER;
    node_info.node_id_ = node_id;
    node_info.rank_id_ = rank_id;
    nodes_info_[node_id] = node_info;
    MS_LOG(INFO) << "The worker node id:" << node_id << " assign rank id:" << rank_id;
  }
  return rank_id;
}

void SchedulerNode::ProcessFinish(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  FinishMessage finish_message;
  finish_message.ParseFromString(message.data());
  finish_nodes_id_.insert(finish_message.node_id());
  const_cast<TcpServer &>(server).SendMessage(conn, message);
}

void SchedulerNode::ProcessFetchServers(const TcpServer &server, const TcpConnection &conn,
                                        const CommMessage &message) {
  FetchServersRespMessage fetch_servers_message;
  std::vector<ServersMeta> servers_meta_list;
  for (auto it = nodes_info_.begin(); it != nodes_info_.end(); ++it) {
    if (it->second.node_role_ == NodeRole::SERVER) {
      ServersMeta servers_meta;
      servers_meta.set_rank_id(it->second.rank_id_);
      servers_meta.set_ip(it->second.ip_);
      servers_meta.set_port(it->second.port_);
      servers_meta_list.push_back(servers_meta);
    }
  }

  *fetch_servers_message.mutable_servers_meta() = {servers_meta_list.begin(), servers_meta_list.end()};

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message.pb_meta()};
  comm_message.set_data(fetch_servers_message.SerializeAsString());
  const_cast<TcpServer &>(server).SendMessage(conn, comm_message);
}

void SchedulerNode::StartClusterAvailableTimer() {
  MS_LOG(WARNING) << "The scheduler start a timing event to determine whether the system is available";
  server_->set_timer_once_callback([&](const TcpServer &server) {
    uint32_t total_node_num = ClusterConfig::server_num() + ClusterConfig::worker_num();
    if (total_node_num != nodes_info_.size()) {
      MS_LOG(WARNING) << "The cluster is not ready after " << ClusterConfig::cluster_available_timeout()
                      << " seconds,so quit the cluster!";
      if (on_node_event_message_) {
        on_node_event_message_(NodeEvent::NODE_TIMEOUT);
      }
    }
  });
  server_->StartTimerOnlyOnce(ClusterConfig::cluster_available_timeout());
}

void SchedulerNode::StartHeartbeatTimer() {
  MS_LOG(WARNING) << "The scheduler start a heartbeat timer!";
  uint32_t total_node_num = ClusterConfig::server_num() + ClusterConfig::worker_num();
  server_->set_timer_callback([&]() {
    // 1. assign node timeout
    struct timeval current_time {};
    (void)gettimeofday(&current_time, nullptr);
    timeout_nodes_info_.clear();
    for (auto it = heartbeats_.begin(); it != heartbeats_.end(); ++it) {
      if (it->second.tv_sec + ClusterConfig::heartbeat_timeout() < current_time.tv_sec) {
        MS_LOG(ERROR) << "The node id:" << it->first << " is timeout!";
        timeout_nodes_info_[it->first] = nodes_info_[it->first];
      }
    }
    if (!timeout_nodes_info_.empty()) {
      is_node_timeout_ = true;
      for (auto it = timeout_nodes_info_.begin(); it != timeout_nodes_info_.end(); ++it) {
        finish_nodes_id_.insert(it->first);
      }
    }

    // 2. assign node finish
    if (finish_nodes_id_.size() == total_node_num) {
      is_node_finish_ = true;
    }
  });
  server_->StartTimer(ClusterConfig::heartbeat_interval());

  // 3. assign node ready
  if (nodes_info_.size() == total_node_num) {
    is_cluster_ready_ = true;
  }
}

void SchedulerNode::Stop() {
  MS_LOG(INFO) << "Stop scheduler node!";
  if (!is_node_stop_) {
    is_node_stop_ = true;
    server_->Stop();
    if (scheduler_thread_->joinable()) {
      scheduler_thread_->join();
    }
    is_cluster_ready_ = true;
  }
}

}  // namespace core
}  // namespace ps
}  // namespace mindspore