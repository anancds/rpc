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
  if (!is_already_stopped_.load()) {
    server_->Stop();
    client_to_scheduler_->Stop();
    client_to_scheduler_->StopEventBase();
    if (server_thread_->joinable()) {
      server_thread_->join();
    }
    if (client_to_scheduler_thread_->joinable()) {
      client_to_scheduler_thread_->join();
    }
    is_already_stopped_ = true;
  }
}

void ServerNode::Start() {
  MS_LOG(INFO) << "Start server node!";
  Init();
  Initialize();
  InitClientToScheduler();
  Register(client_to_scheduler_);
  Heartbeat(client_to_scheduler_);

  WaitForStart();
  MS_LOG(INFO) << "The cluster is ready to use!";

  if (!is_timeout_) {
    MS_LOG(INFO) << "The node begin to fetch servers";
    FetchServers(client_to_scheduler_);
    MS_LOG(INFO) << "Fetch servers successful!";
  }
  MS_LOG(INFO) << "Start the node is successful!";
}

void ServerNode::Register(const std::shared_ptr<TcpClient> &client) {
  MessageMeta message_meta;
  message_meta.set_cmd(NodeCommand::REGISTER);

  RegisterMessage register_message;
  register_message.set_node_id(node_info_.node_id_);
  register_message.set_role(node_info_.node_role_);
  register_message.set_ip(node_info_.ip_);
  register_message.set_port(node_info_.port_);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message_meta};
  comm_message.set_data(register_message.SerializeAsString());
  SendMessageSync(client, comm_message);

  MS_LOG(INFO) << "The server node id:" << node_info_.node_id_
               << "is registering to scheduler, the request id is:" << message_meta.request_id();
}

void ServerNode::set_handler(const RequestHandler &handler) { request_handler_ = handler; }

void ServerNode::Response(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  const_cast<TcpServer &>(server).SendMessage(conn, message);
}

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

void ServerNode::Init() {
  std::string interface;
  std::string server_ip;
  CommUtil::GetAvailableInterfaceAndIP(&interface, &server_ip);
  server_ = std::make_shared<TcpServer>(server_ip, 0);
  server_->SetMessageCallback([&](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
    switch (message.pb_meta().cmd()) {
      case NodeCommand::SEND_DATA:
        ProcessSendData(server, conn, message);
        break;
      default:
        MS_LOG(EXCEPTION) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
  });
  server_->Init();
  server_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The server node start a tcp server!";
    server_->Start();
  });
  server_thread_->detach();
}

void ServerNode::Initialize() {
  is_already_stopped_ = false;
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
      case NodeCommand::REGISTER:
        ProcessRegister(message);
        break;
      case NodeCommand::HEARTBEAT:
        ProcessHeartbeatResp(message);
        break;
      case NodeCommand::FETCH_SERVER:
        ProcessFetchServersResp(message);
        break;
      case NodeCommand::FINISH:
        MS_LOG(INFO) << "The Node id:" << node_info_.node_id_ << " receive a finish message response!";
        break;
      default:
        MS_LOG(EXCEPTION) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
    NotifyMessageArrival(message);
  });

  client_to_scheduler_->Init();
  client_to_scheduler_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The server node start a tcp client!";
    client_to_scheduler_->Start();
  });
  client_to_scheduler_thread_->detach();
}

void ServerNode::ProcessSendData(const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
  if (request_handler_) {
    request_handler_(server, conn, message);
  }
}

void ServerNode::Stop() {
  MS_LOG(INFO) << "Stop server node!";
  if (!is_already_stopped_.load()) {
    server_->Stop();
    client_to_scheduler_->Stop();
    client_to_scheduler_->StopEventBase();
    if (server_thread_->joinable()) {
      server_thread_->join();
    }
    if (client_to_scheduler_thread_->joinable()) {
      client_to_scheduler_thread_->join();
    }
    if (heart_beat_thread_->joinable()) {
      heart_beat_thread_->join();
    }
    is_already_stopped_ = true;
  }
}

void ServerNode::Finish() {
  std::lock_guard<std::mutex> lock(finish_mutex_);
  if (is_already_finished_) {
    MS_LOG(INFO) << "Server node already finish!";
    return;
  }
  MS_LOG(INFO) << "Finish server node begin !";
  Disconnect(client_to_scheduler_);
  is_already_finished_ = true;
  MS_LOG(INFO) << "Finish server node end!";
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore