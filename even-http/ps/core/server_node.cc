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
  Stop();
}

bool ServerNode::Start(const uint32_t &timeout) {
  MS_LOG(INFO) << "Start server node!";
  Initialize();
  Register(client_to_scheduler_);
  StartHeartbeatTimer(client_to_scheduler_);

  if (!WaitForStart(timeout)) {
    MS_LOG(ERROR) << "Start server node timeout!";
    return false;
  }
  MS_LOG(INFO) << "The cluster is ready to use!";

  // If the cluster is ready to use, then Get the address of all the servers
  if (!is_timeout_.load()) {
    FetchServers(client_to_scheduler_);
    MS_LOG(INFO) << "Server node get all the servers address successful!";
  }
  MS_LOG(INFO) << "Start the node is successful!";
  return true;
}

void ServerNode::set_handler(const RequestHandler &handler) { request_handler_ = handler; }

void ServerNode::Response(std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> message) {
  message->mutable_pb_meta()->set_role(node_info_.node_role_);
  message->mutable_pb_meta()->set_rank_id(node_info_.rank_id_);
  server_->SendMessage(conn, message);
}

void ServerNode::CreateTcpServer() {
  std::string interface;
  std::string server_ip;
  CommUtil::GetAvailableInterfaceAndIP(&interface, &server_ip);
  server_ = std::make_shared<TcpServer>(server_ip, 0);
  server_->SetMessageCallback([&](std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> message) {
    switch (message->pb_meta().cmd()) {
      case NodeCommand::SEND_DATA:
        ProcessSendData(conn, message);
        break;
      case NodeCommand::COLLECTIVE_SEND_DATA:
        ProcessCollectiveSendData(conn, message);
        RunReceiveCallback(*message);
        break;
      default:
        MS_LOG(EXCEPTION) << "The cmd:" << message->pb_meta().cmd() << " is not supported!";
    }
  });
  server_->Init();
  server_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The server node start a tcp server!";
    server_->Start();
  });
}

void ServerNode::Initialize() {
  CreateTcpServer();
  is_already_stopped_ = false;
  node_info_.node_id_ = CommUtil::GenerateUUID();
  node_info_.node_role_ = NodeRole::SERVER;
  node_info_.ip_ = server_->BoundIp();
  node_info_.port_ = server_->BoundPort();
  MS_LOG(INFO) << "The node role:" << CommUtil::NodeRoleToString(node_info_.node_role_)
               << " is generate uuid is:" << node_info_.node_id_;
  if (!InitClientToScheduler()) {
    MS_LOG(EXCEPTION) << "Server node init client timeout!";
  }
  MS_LOG(INFO) << "Server node init client successful!";
}

void ServerNode::ProcessSendData(std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> message) {
  request_handler_(conn, message);
}

void ServerNode::ProcessCollectiveSendData(std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> message) {
  std::shared_ptr<CommMessage> comm_message = std::make_shared<CommMessage>();
  *comm_message->mutable_pb_meta() = {message->pb_meta()};
  server_->SendMessage(conn, comm_message);
}

bool ServerNode::Stop() {
  MS_LOG(INFO) << "Stop server node!";
  if (!is_already_stopped_.load()) {
    is_already_stopped_ = true;
    is_finish_ = true;
    heart_beat_thread_->join();
    client_to_scheduler_->Stop();
    if (!connected_nodes_.empty()) {
      for (auto &connected_node : connected_nodes_) {
        connected_node.second->Stop();
      }
    }
    client_to_scheduler_thread_->join();
    server_->Stop();
    server_thread_->join();
  }
  return true;
}

bool ServerNode::Finish(const uint32_t &timeout) {
  std::lock_guard<std::mutex> lock(finish_mutex_);
  if (is_already_finished_) {
    MS_LOG(INFO) << "Server node already finish!";
    return true;
  }
  is_already_finished_ = true;
  return Disconnect(client_to_scheduler_, timeout);
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore
