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
#include "ps/core/server_node.h"

namespace mindspore {
namespace ps {
namespace core {

void ServerNode::Start() {
  MS_LOG(INFO) << "Start server node!";
  std::string interface;
  std::string server_ip;
  CommUtil::GetAvailableInterfaceAndIP(&interface, &server_ip);
  server_ = std::make_shared<TcpServer>(server_ip, 0);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {});
  server_->Init();
  server_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The server node start a tcp server!";
    server_->Start();
  });

  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint16_t scheduler_port = ClusterConfig::scheduler_port();
  client_to_scheduler_ = std::make_unique<TcpClient>(scheduler_host, scheduler_port);
  client_to_scheduler_->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    switch (message.pb_meta().cmd()) {
      case ClusterCommand::TERMINATE:
        ProcessTerminal(message);
        break;
      case ClusterCommand::REGISTER:
        ProcessRegister(message);
        break;
      case ClusterCommand::HEARTBEAT:
        ProcessHeartbeat(message);
        break;
      default:
        MS_LOG(INFO) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
  });

  is_node_stop_ = false;
  client_to_scheduler_->Init();

  client_to_scheduler_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The server node start a tcp client!";
    client_to_scheduler_->Start();
  });

  Register(client_to_scheduler_, server_ip, server_->BoundPort(), NodeRole::SERVER);

  while (!is_cluster_ready_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  MS_LOG(INFO) << "The cluster is ready to use!";
  Heartbeat(client_to_scheduler_);
}

void ServerNode::Send(const enum NodeRole &node_role, uint32_t rank_id, const CommMessage &message) {
  if (node_role != NodeRole::SERVER) {
    MS_LOG(EXCEPTION) << "The node role should be SERVER!";
  }
  if (rank_id > ClusterConfig::server_num() - 1) {
    MS_LOG(EXCEPTION) << "The rank id:" << rank_id << " is illegal!";
  }
  auto client = GetOrCreateTcpClient(rank_id);
  client->SendMessage(message);
}

void ServerNode::Register(const std::shared_ptr<TcpClient> &client, const std::string &host, const uint32_t &port,
                          const NodeRole &role) {
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(ClusterCommand::REGISTER);
  message_meta.set_hostname(host);
  message_meta.set_port(port);
  message_meta.set_role(role);
  message_meta.set_node_id(node_id_);
  *comm_message.mutable_pb_meta() = {message_meta};
  client->SendMessage(comm_message);
}

void ServerNode::ProcessRegister(const CommMessage &message) {
  RegisterMessage register_message;
  register_message.ParseFromString(message.data());
  if (register_message.node_id().compare(node_id_)) {
    rank_id_ = register_message.rank_id();
    if (on_node_event_message_) {
      on_node_event_message_(NodeEvent::REGISTER_SUCCESS);
    }
  }
  MS_LOG(INFO) << "The server node id is:" << node_id_ << ", and the rank id is:" << rank_id_;

  is_cluster_ready_ = message.pb_meta().is_cluster_ready();

  if (is_cluster_ready_) {
    auto meta_begin = register_message.servers_meta().begin();
    auto meta_end = register_message.servers_meta().end();
    for (auto it = meta_begin; it != meta_end; ++it) {
      server_node_rank_ids_[it->rank_id()] = std::make_pair(it->node_id(), it->server_host());
    }

    if (on_node_event_message_) {
      on_node_event_message_(NodeEvent::CLUSTER_READY);
    }

    MS_LOG(DEBUG) << "The all server host size is:" << server_node_rank_ids_.size();
  }
}

void ServerNode::ProcessTerminal(const CommMessage &message) {
  MS_LOG(INFO) << "The node role: " << node_role_ << ", the node id:" << node_id_ << ", the node rank id:" << rank_id_
               << " is process terminal message!";
  if (on_node_event_message_) {
    on_node_event_message_(NodeEvent::TERMINATE_READY);
  }
}

const std::shared_ptr<TcpClient> &ServerNode::GetOrCreateTcpClient(const int &rank_id) {
  std::lock_guard<std::mutex> lock(client_mutex_);
  if (connected_nodes_.find(rank_id) != connected_nodes_.end()) {
    return connected_nodes_[rank_id];
  } else {
    if (server_node_rank_ids_.find(rank_id) == server_node_rank_ids_.end()) {
      MS_LOG(EXCEPTION) << "Server node Fetch servers failed!";
    }
    std::string host_and_port = server_node_rank_ids_[rank_id].second;
    int index = host_and_port.find(":");
    std::string host = host_and_port.substr(0, index);
    uint16_t port = std::strtol(host_and_port.substr(index + 1, host_and_port.size()).c_str(), nullptr, 10);
    auto client = std::make_shared<TcpClient>(host, port);
    connected_nodes_[rank_id] = client;
    return connected_nodes_[rank_id];
  }
}

void ServerNode::Stop() {
  MS_LOG(INFO) << "Stop server node!";
  if (!is_node_stop_.load()) {
    server_->Stop();
    client_to_scheduler_->Stop();
    client_to_scheduler_->StopEventBase();
    server_thread_->join();
    client_to_scheduler_thread_->join();
    is_node_stop_ = true;
  }
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore