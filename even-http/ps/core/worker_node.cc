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
  if (!is_node_stop_.load()) {
    client_to_scheduler_->Stop();
    if (worker_thread_->joinable()) {
      worker_thread_->join();
    }
    is_node_stop_ = true;
    test_ = false;
  }
}
void WorkerNode::Start() {
  MS_LOG(INFO) << "Start worker node!";
  std::string scheduler_host = ClusterConfig::scheduler_host();
  uint16_t scheduler_port = ClusterConfig::scheduler_port();
  client_to_scheduler_ = std::make_shared<TcpClient>(scheduler_host, scheduler_port);
  client_to_scheduler_->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    switch (message.pb_meta().cmd()) {
      case NodeCommand::HEARTBEAT:
        ProcessHeartbeat(message);
        break;
      case NodeCommand::TERMINATE:
        ProcessTerminate(message);
        break;
      case NodeCommand::REGISTER:
        ProcessRegister(message);
        break;
      case NodeCommand::SEND_DATA:
        ProcessData(message);
        break;
      default:
        MS_LOG(INFO) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
    }
  });

  is_node_stop_ = false;
  node_id_ = CommUtil::GenerateUUID();
  node_role_ = NodeRole::WORKER;
  client_to_scheduler_->Init();
  MS_LOG(INFO) << "The node role is:" << CommUtil::NodeRoleToString(node_role_) << ", the node id is:" << node_id_;

  worker_thread_ = std::make_unique<std::thread>([&]() {
    MS_LOG(INFO) << "The worker node start a tcp client!";
    client_to_scheduler_->Start();
  });
  worker_thread_->detach();

  Register(client_to_scheduler_, NodeRole::WORKER);

  Heartbeat(client_to_scheduler_);

  while (!is_cluster_ready_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  MS_LOG(INFO) << "The cluster is ready to use!";
  while (test_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

void WorkerNode::Register(const std::shared_ptr<TcpClient> &client, const NodeRole &role) {
  MessageMeta message_meta;
  CommMessage comm_message;
  message_meta.set_cmd(ClusterCommand::REGISTER);
  message_meta.set_role(role);
  message_meta.set_node_id(node_id_);
  *comm_message.mutable_pb_meta() = {message_meta};
  client->SendMessage(comm_message);
}

void WorkerNode::Send(const enum NodeRole &node_role, uint32_t rank_id, CommMessage &message) {
  if (node_role != NodeRole::SERVER) {
    MS_LOG(EXCEPTION) << "The node role should be SERVER!";
  }
  if (rank_id > ClusterConfig::server_num() - 1) {
    MS_LOG(EXCEPTION) << "The rank id:" << rank_id << " is illegal!";
  }

  uint64_t timestamp = AssignTimestamp(1);
  MessageMeta message_meta;
  message_meta.set_cmd(ClusterCommand::SEND_DATA);
  message_meta.set_role(node_role_);
  message_meta.set_node_id(node_id_);
  message_meta.set_request_id(timestamp);
  *message.mutable_pb_meta() = {message_meta};

  auto client = GetOrCreateTcpClient(rank_id);
  client->SendMessage(message);
}

void WorkerNode::Wait(uint64_t timestamp) {
  std::unique_lock<std::mutex> lock(message_mutex_);
  message_tracker_cond_.wait(lock,
                             [&] { return message_tracker_[timestamp].first == message_tracker_[timestamp].second; });
}

void WorkerNode::ProcessRegister(const CommMessage &message) {
  RegisterMessage register_message;
  register_message.ParseFromString(message.data());
  if (register_message.node_id() == node_id_) {
    rank_id_ = register_message.rank_id();
    if (on_node_event_message_) {
      on_node_event_message_(NodeEvent::REGISTER_SUCCESS);
    }
  }
  MS_LOG(INFO) << "The client node id is:" << node_id_ << ", and the rank id is:" << rank_id_;

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

void WorkerNode::ProcessTerminate(const CommMessage &message) {
  MS_LOG(INFO) << "The node role: " << node_role_ << ", the node id:" << node_id_ << ", the node rank id:" << rank_id_
               << " is process terminal message!";
  if (on_node_event_message_) {
    on_node_event_message_(NodeEvent::TERMINATE_READY);
  }
}

void WorkerNode::ProcessData(const CommMessage &message) {
  std::lock_guard<std::mutex> lock(message_mutex_);
  message_tracker_[message.pb_meta().request_id()].second++;
  message_tracker_cond_.notify_all();
}

const std::shared_ptr<TcpClient> &WorkerNode::GetOrCreateTcpClient(const int &rank_id) {
  std::lock_guard<std::mutex> lock(client_mutex_);
  if (connected_nodes_.find(rank_id) != connected_nodes_.end()) {
    return connected_nodes_[rank_id];
  } else {
    if (server_node_rank_ids_.find(rank_id) == server_node_rank_ids_.end()) {
      MS_LOG(EXCEPTION) << "Worker node Fetch servers failed!";
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

uint64_t WorkerNode::AssignTimestamp(const uint32_t &server_sent_num) {
  std::lock_guard<std::mutex> lock(message_mutex_);
  uint64_t timestamp = ++timestamp_;
  message_tracker_[timestamp] = std::make_pair(server_sent_num, 0);
  return timestamp;
}

void WorkerNode::Stop() {
  MS_LOG(INFO) << "Stop worker node!";
  if (!is_node_stop_.load()) {
    client_to_scheduler_->Stop();
    if (worker_thread_->joinable()) {
      worker_thread_->join();
    }
    is_node_stop_ = true;
    test_ = false;
  }
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore
