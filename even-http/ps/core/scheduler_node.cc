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

void SchedulerNode::Start() {
  MS_LOG(INFO) << "Start scheduler node!";
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint32_t scheduler_port = ClusterConfig::scheduler_port();

  server_ = std::make_unique<TcpServer>(scheduler_host, scheduler_port);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
    switch (message.pb_meta().cmd()) {
      case ClusterCommand::HEARTBEAT:
        ProcessHeartBeat(server, conn, message);
        break;
      case ClusterCommand::REGISTER:
        ProcessRegister(server, conn, message);
        break;
      default:
        MS_LOG(INFO) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
  });

  is_node_stop_ = false;
  server_->Init();
  node_id_ = CommUtil::GenerateUUID();

  scheduler_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The scheduler node start a tcp server!";
    server_->Start();
  });

  StartClusterAvailableTimer();

  //改成bool，是不需要发送terminal指令
  while (!is_cluster_ready_.load() || server_->ConnectionNum() != 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  MS_LOG(INFO) << "The scheduler is ready to quit!";
}

void SchedulerNode::HeartBeat(const TcpServer &server, const TcpConnection &conn) {
  MessageMeta message_meta;
  message_meta.set_cmd(ClusterCommand::HEARTBEAT);
  message_meta.set_role(NodeRole::SCHEDULER);
  message_meta.set_hostname(ClusterConfig::scheduler_host());
  message_meta.set_port(ClusterConfig::scheduler_port());
  message_meta.set_node_id(node_id_);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message_meta};
  const_cast<TcpServer&>(server).SendMessage(conn, comm_message);
}

void SchedulerNode::ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  const MessageMeta &message_meta = message.pb_meta();
  struct timeval current_time {};
  (void)gettimeofday(&current_time, nullptr);
  UpdateHeartbeat(message_meta.node_id(), current_time);

  HeartBeat(server, conn);

  for (auto it = heartbeats_.begin(); it != heartbeats_.end(); ++it) {
    if (it->second.tv_sec + ClusterConfig::heartbeat_timeout() < current_time.tv_sec) {
      MS_LOG(ERROR) << "There is a node failed, should terminal the whole cluster!";
      Terminal(server);
    }
  }
}

void SchedulerNode::ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  MS_LOG(INFO) << "The scheduler process a register message!";
  // assign worker node and server node rank id
  int rank_id = AssignRankId(message);
  std::string node_id = message.pb_meta().node_id();

  uint32_t total_node_num = ClusterConfig::server_num() + ClusterConfig::worker_num();
  if (total_node_num == (worker_nodes_.size() + server_nodes_.size())) {
    MS_LOG(INFO) << "All worker nodes and server nodes have registered to scheduler!";

    RegisterResponse(server, conn, rank_id, node_id, true);
  } else {
    RegisterResponse(server, conn, rank_id, node_id, false);
  }
}

void SchedulerNode::RegisterResponse(const TcpServer &server, const TcpConnection &conn, const int &rank_id,
                                     const std::string &node_id, bool is_cluster_ready) {
  struct timeval current_time {};
  (void)gettimeofday(&current_time, nullptr);
  UpdateHeartbeat(node_id, current_time);

  MessageMeta message_meta;
  message_meta.set_cmd(ClusterCommand::REGISTER);
  message_meta.set_role(NodeRole::SCHEDULER);
  message_meta.set_hostname(ClusterConfig::scheduler_host());
  message_meta.set_port(ClusterConfig::scheduler_port());
  message_meta.set_node_id(node_id_);
  is_cluster_ready_ = is_cluster_ready;
  message_meta.set_is_cluster_ready(is_cluster_ready);

  RegisterMessage register_message;
  register_message.set_node_id(node_id);
  register_message.set_rank_id(rank_id);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message_meta};

  if (is_cluster_ready) {
    std::vector<ServersMeta> servers_meta_list;
    for (auto it = server_nodes_.begin(); it != server_nodes_.end(); ++it) {
      ServersMeta servers_meta;
      servers_meta.set_node_id(it->first);
      servers_meta.set_rank_id(it->second.first);
      servers_meta.set_server_host(it->second.second);
      servers_meta_list.push_back(servers_meta);
    }

    *register_message.mutable_servers_meta() = {servers_meta_list.begin(), servers_meta_list.end()};

    comm_message.set_data(register_message.SerializeAsString());
    const_cast<TcpServer&>(server).SendMessage(comm_message);
  } else {
    comm_message.set_data(register_message.SerializeAsString());
    const_cast<TcpServer&>(server).SendMessage(conn, comm_message);
  }
}

int SchedulerNode::AssignRankId(const CommMessage &message) {
  std::lock_guard<std::mutex> lock(assign_rank_id_mutex_);
  int rank_id = -1;
  if (message.pb_meta().role() == NodeRole::SERVER) {
    std::string ip = message.pb_meta().hostname();
    uint32_t port = message.pb_meta().port();
    std::string node_id = message.pb_meta().node_id();
    std::string node_host_ip_and_port = ip + ":" + std::to_string(port);
    if (server_nodes_.find(node_id) != server_nodes_.end()) {
      rank_id = server_nodes_[node_id].first;
    } else {
      rank_id = ++current_server_rank_id_;
      server_nodes_[node_id] = std::make_pair(rank_id, node_host_ip_and_port);
    }
    MS_LOG(INFO) << "The server node host: " << node_host_ip_and_port << " node id: " << node_id
                 << " assign rank id:" << rank_id;
  } else if (message.pb_meta().role() == NodeRole::SERVER) {
    std::string node_id = message.pb_meta().node_id();
    if (worker_nodes_.find(node_id) != worker_nodes_.end()) {
      rank_id = worker_nodes_[node_id];
    } else {
      rank_id = ++current_worker_rank_id_;
      worker_nodes_[node_id] = rank_id;
    }
    MS_LOG(INFO) << "The worker node id: " << node_id << " assign rank id:" << rank_id;
  }
  return rank_id;
}

void SchedulerNode::Terminal(const TcpServer &server) {
  MessageMeta message_meta;
  message_meta.set_cmd(ClusterCommand::TERMINATE);
  message_meta.set_role(NodeRole::SCHEDULER);
  message_meta.set_hostname(ClusterConfig::scheduler_host());
  message_meta.set_port(ClusterConfig::scheduler_port());
  message_meta.set_node_id(node_id_);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message_meta};
  const_cast<TcpServer &>(server).SendMessage(comm_message);
}

void SchedulerNode::StartClusterAvailableTimer() {
  MS_LOG(WARNING) << "The scheduler start a timing event to determine whether the system is available";
  server_->set_timer_callback([&](const TcpServer &server) {
    uint32_t total_node_num = ClusterConfig::server_num() + ClusterConfig::worker_num();
    if (total_node_num != (worker_nodes_.size() + server_nodes_.size())) {
      MS_LOG(WARNING) << "The cluster is not ready after " << ClusterConfig::cluster_available_timeout()
                      << " seconds,so quit the cluster!";
      if (on_node_event_message_) {
        on_node_event_message_(NodeEvent::CLUSTER_TIMEOUT);
      }
      Terminal(server);
    }
  });
  server_->StartTimerOnlyOnce(ClusterConfig::cluster_available_timeout());
}

void SchedulerNode::Stop() {
  MS_LOG(INFO) << "Stop scheduler node!";
  if (!is_node_stop_) {
    is_node_stop_ = true;
    server_->Stop();
    scheduler_thread_->join();
  }
}

}  // namespace core
}  // namespace ps
}  // namespace mindspore