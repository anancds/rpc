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

#include "ps/core/worker_node.h"

namespace mindspore {
namespace ps {
namespace core {
WorkerNode::~WorkerNode() {
  MS_LOG(INFO) << "Stop worker node!";
  if (!is_already_stopped_.load()) {
    client_to_scheduler_->Stop();
    if (worker_thread_->joinable()) {
      worker_thread_->join();
    }
    is_already_stopped_ = true;
  }
}
void WorkerNode::Start() {
  MS_LOG(INFO) << "Start worker node!";
  InitNode();
  InitClientToScheduler();
  // 注册接口也应该是同步接口
  Register();
  Heartbeat(client_to_scheduler_);

  WaitForStart();
  MS_LOG(INFO) << "The node is ready to fetch servers!";

  if (!is_timeout_.load()) {
    FetchServers(client_to_scheduler_);
    MS_LOG(INFO) << "Fetch servers successful!";
  }
  MS_LOG(INFO) << "Start the node is successful!";
}

void WorkerNode::Register() {
  MessageMeta message_meta;
  message_meta.set_cmd(NodeCommand::REGISTER);
  message_meta.set_request_id(++next_request_id_);

  RegisterMessage register_message;
  register_message.set_node_id(node_info_.node_id_);
  register_message.set_role(node_info_.node_role_);

  CommMessage comm_message;
  *comm_message.mutable_pb_meta() = {message_meta};
  comm_message.set_data(register_message.SerializeAsString());
  client_to_scheduler_->SendMessage(comm_message);
  MS_LOG(INFO) << "The worker node id:" << node_info_.node_id_
               << "is registering to scheduler, the request id is:" << message_meta.request_id();
}

void WorkerNode::Send(const enum NodeRole &node_role, const uint32_t &rank_id, CommMessage &message) {
  if (!CommUtil::CheckRoleAndRankId(node_role, rank_id)) {
    MS_LOG(ERROR) << "The node role or rank_id is illegal!";
  }

  MessageMeta message_meta;
  message_meta.set_cmd(NodeCommand::SEND_DATA);
  *message.mutable_pb_meta() = {message_meta};

  auto client = GetOrCreateTcpClient(rank_id);
  SendMessageSync(client, message);
}

void WorkerNode::Send(const std::vector<std::tuple<const enum NodeRole &, const uint32_t &, CommMessage &>> &data) {
  uint64_t request_id = ++next_request_id_;
  message_tracker_[request_id] = std::make_pair(data.size(), 0);
  for (auto it = data.begin(); it != data.end(); ++it) {
    NodeRole node_role;
    uint32_t rank_id;
    CommMessage comm_message;
    std::tie(node_role, rank_id, comm_message) = *it;

    if (!CommUtil::CheckRoleAndRankId(node_role, rank_id)) {
      MS_LOG(ERROR) << "The node role or rank_id is illegal!";
    }

    MessageMeta message_meta;
    message_meta.set_cmd(NodeCommand::SEND_DATA);
    message_meta.set_request_id(request_id);
    *comm_message.mutable_pb_meta() = {message_meta};
    auto client = GetOrCreateTcpClient(rank_id);
    client->SendMessage(comm_message);
  }
  Wait(request_id);
}

void WorkerNode::BroadCast(CommMessage &message) {
  uint64_t request_id = ++next_request_id_;
  message_tracker_[request_id] = std::make_pair(server_rank_ids_.size(), 0);
  for (auto it = server_rank_ids_.begin(); it != server_rank_ids_.end(); ++it) {

    MessageMeta message_meta;
    message_meta.set_cmd(NodeCommand::SEND_DATA);
    message_meta.set_request_id(request_id);
    *message.mutable_pb_meta() = {message_meta};
    auto client = GetOrCreateTcpClient((*it).first);
    client->SendMessage(message);
  }
  Wait(request_id);
}

void WorkerNode::ProcessRegister(const CommMessage &message) {
  RegisterRespMessage register_resp_message;
  register_resp_message.ParseFromString(message.data());
  if (register_resp_message.node_id() != node_info_.node_id_) {
    MS_LOG(EXCEPTION) << "The node id received:" << register_resp_message.node_id()
                      << " is not match the current node id:" << node_info_.node_id_;
  }

  node_info_.rank_id_ = register_resp_message.rank_id();

  MS_LOG(INFO) << "The client node id is:" << node_info_.node_id_ << ", and the rank id is:" << node_info_.rank_id_;
}

void WorkerNode::ProcessData(const CommMessage &message) {}

const std::shared_ptr<TcpClient> &WorkerNode::GetOrCreateTcpClient(const int &rank_id) {
  std::lock_guard<std::mutex> lock(client_mutex_);
  if (connected_nodes_.find(rank_id) != connected_nodes_.end()) {
    return connected_nodes_[rank_id];
  } else {
    if (server_rank_ids_.find(rank_id) == server_rank_ids_.end()) {
      MS_LOG(EXCEPTION) << "Worker node Fetch servers failed!";
    }
    std::string ip = server_rank_ids_[rank_id].first;
    uint16_t port = server_rank_ids_[rank_id].second;
    auto client = std::make_shared<TcpClient>(ip, port);
    client->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
      switch (message.pb_meta().cmd()) {
        case NodeCommand::SEND_DATA:
          ProcessData(message);
          break;
        default:
          MS_LOG(INFO) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
      }
      NotifyMessageArrival(message);
    });
    client->Init();
    connected_nodes_[rank_id] = client;
    return connected_nodes_[rank_id];
  }
}

void WorkerNode::InitNode() {
  is_already_stopped_ = false;
  node_info_.node_id_ = CommUtil::GenerateUUID();
  node_info_.node_role_ = NodeRole::WORKER;
  MS_LOG(INFO) << "The node role is:" << CommUtil::NodeRoleToString(node_info_.node_role_)
               << ", the node id is:" << node_info_.node_id_;
}

void WorkerNode::InitClientToScheduler() {
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint16_t scheduler_port = ClusterConfig::scheduler_port();
  client_to_scheduler_ = std::make_shared<TcpClient>(scheduler_host, scheduler_port);
  client_to_scheduler_->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    switch (message.pb_meta().cmd()) {
      case NodeCommand::HEARTBEAT:
        ProcessHeartbeatResp(message);
        break;
      case NodeCommand::REGISTER:
        ProcessRegister(message);
        break;
      case NodeCommand::FETCH_SERVER:
        ProcessFetchServersResp(message);
        break;
      case NodeCommand::FINISH:
        MS_LOG(INFO) << "The Node id:" << node_info_.node_id_ << " receive a finish message response!";
        break;
      default:
        MS_LOG(INFO) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
    NotifyMessageArrival(message);
  });

  client_to_scheduler_->Init();
  worker_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The worker node start a tcp client!";
    client_to_scheduler_->Start();
  });
  worker_thread_->detach();
}

void WorkerNode::Stop() {
  MS_LOG(INFO) << "Stop worker node!";
  if (!is_already_stopped_.load()) {
    is_ready_ = true;
    is_timeout_ = true;
    client_to_scheduler_->Stop();
    client_to_scheduler_->StopEventBase();
    if (worker_thread_->joinable()) {
      worker_thread_->join();
    }
    if (heart_beat_thread_->joinable()) {
      heart_beat_thread_->join();
    }
    is_already_stopped_ = true;
  }
}

void WorkerNode::Finish() {
  std::lock_guard<std::mutex> lock(finish_mutex_);
  if (is_already_finished_) {
    MS_LOG(INFO) << "Worker node already finish!";
    return;
  }
  MS_LOG(INFO) << "Finish worker node!";
  Disconnect(client_to_scheduler_);
  is_already_finished_ = true;
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore
