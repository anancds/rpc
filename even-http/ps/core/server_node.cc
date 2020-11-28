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
ServerNode::~ServerNode() {
  MS_LOG(INFO) << "Stop server node!";
  if (!is_node_stop_.load()) {
    server_->Stop();
    client_to_scheduler_->Stop();
    client_to_scheduler_->StopEventBase();
    if (server_thread_->joinable()) {
      server_thread_->join();
    }
    if (client_to_scheduler_thread_->joinable()) {
      client_to_scheduler_thread_->join();
    }
    is_node_stop_ = true;
  }
}

void ServerNode::Start() {
  MS_LOG(INFO) << "Start server node!";
  Init();
  InitNode();
  InitClientToScheduler();
  Register(client_to_scheduler_);
  Heartbeat(client_to_scheduler_);

  while (!is_cluster_ready_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  MS_LOG(INFO) << "The cluster is ready to use!";

  Wait(FetchServers(client_to_scheduler_));
  MS_LOG(INFO) << "Fetch servers successful!";
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

void ServerNode::Register(const std::shared_ptr<TcpClient> &client) {
  MessageMeta message_meta;
  message_meta.set_cmd(NodeCommand::REGISTER);
  message_meta.set_request_id(++request_id_);

  RegisterMessage register_message;
  register_message.set_node_id(node_info_.node_id_);
  register_message.set_role(node_info_.node_role_);
  register_message.set_ip(node_info_.ip_);
  register_message.set_port(node_info_.port_);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message_meta};
  comm_message.set_data(register_message.SerializeAsString());
  client->SendMessage(comm_message);

  MS_LOG(INFO) << "The server node id:" << node_info_.node_id_
               << "is registering to scheduler, the request id is:" << message_meta.request_id();
}

void ServerNode::set_handler(const RequestHandler &handler) { request_handler_ = handler; }

void ServerNode::ProcessRegister(const CommMessage &message) {
  RegisterRespMessage register_resp_message;
  register_resp_message.ParseFromString(message.data());
  if (register_resp_message.node_id() != node_info_.node_id_) {
    MS_LOG(EXCEPTION) << "The node id received:" << register_resp_message.node_id()
                      << " is not match the current node id:" << node_info_.node_id_;
  }

  node_info_.rank_id_ = register_resp_message.rank_id();

  MS_LOG(INFO) << "The server node id is:" << node_info_.node_id_ << ", and the rank id is:" << node_info_.rank_id_;
}

void ServerNode::ProcessTerminal(const CommMessage &message) {
  MS_LOG(INFO) << "The node role: " << node_role_ << ", the node id:" << node_id_ << ", the node rank id:" << rank_id_
               << " is process terminal message!";
  if (on_node_event_message_) {
    on_node_event_message_(NodeEvent::NODE_TIMEOUT);
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
    int index = host_and_port.find(':');
    std::string host = host_and_port.substr(0, index);
    uint16_t port = std::strtol(host_and_port.substr(index + 1, host_and_port.size()).c_str(), nullptr, 10);
    auto client = std::make_shared<TcpClient>(host, port);
    connected_nodes_[rank_id] = client;
    return connected_nodes_[rank_id];
  }
}

void ServerNode::Init() {
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
  server_thread_->detach();
}

void ServerNode::InitNode() {
  is_node_stop_ = false;
  node_info_.node_id_ = CommUtil::GenerateUUID();
  node_info_.node_role_ = NodeRole::SERVER;
  node_info_.ip_ = server_->BoundIp();
  node_info_.port_ = server_->BoundPort();
  MS_LOG(INFO) << "The node role:" << CommUtil::NodeRoleToString(node_info_.node_role_)
               << " is generate uuid is:" << node_info_.node_id_;
}

void ServerNode::InitClientToScheduler() {
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint16_t scheduler_port = ClusterConfig::scheduler_port();
  client_to_scheduler_ = std::make_unique<TcpClient>(scheduler_host, scheduler_port);
  client_to_scheduler_->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    switch (message.pb_meta().cmd()) {
      case NodeCommand::TERMINATE:
        ProcessTerminal(message);
        break;
      case NodeCommand::REGISTER:
        ProcessRegister(message);
        break;
      case NodeCommand::HEARTBEAT:
        ProcessHeartbeat(message);
        break;
      default:
        MS_LOG(INFO) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
  });

  client_to_scheduler_->Init();
  client_to_scheduler_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The server node start a tcp client!";
    client_to_scheduler_->Start();
  });
  client_to_scheduler_thread_->detach();
}

void ServerNode::Stop() {
  MS_LOG(INFO) << "Stop server node!";
  if (!is_node_stop_.load()) {
    server_->Stop();
    client_to_scheduler_->Stop();
    client_to_scheduler_->StopEventBase();
    if (server_thread_->joinable()) {
      server_thread_->join();
    }
    if (client_to_scheduler_thread_->joinable()) {
      client_to_scheduler_thread_->join();
    }
    is_node_stop_ = true;
  }
}

void ServerNode::Finish() {
  MessageMeta meta;
  meta.set_cmd(NodeCommand::FINISH);
  uint64_t request_id = AssignRequestId(1);
  meta.set_request_id(request_id);

  CommMessage message;
  *message.mutable_pb_meta() = {meta};
  client_to_scheduler_->SendMessage(message);

  std::unique_lock<std::mutex> lock(message_mutex_);
  message_tracker_cond_.wait(lock, [&] {
    bool ret = message_tracker_[request_id].first == message_tracker_[request_id].second;
    if (ret) {
      MS_LOG(INFO) << "Message tracker remove request id!";
      message_tracker_.erase(request_id);
    }
    bool res_is_finish = is_cluster_finish_;
    return ret && res_is_finish;
  });
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore