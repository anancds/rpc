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

#include "ps/core/node.h"

namespace mindspore {
namespace ps {
namespace core {
void Node::Heartbeat(const std::shared_ptr<TcpClient> &client) {
  MS_LOG(INFO) << "The node role: " << CommUtil::NodeRoleToString(node_info_.node_role_)
               << ", the node id:" << node_info_.node_id_ << ", the node rank id:" << node_info_.rank_id_
               << " begin send heartbeat to the scheduler!";
  heart_beat_thread_ = std::make_unique<std::thread>([&]() {
    while (!is_finish_.load()) {
      std::this_thread::sleep_for(std::chrono::seconds(ClusterConfig::heartbeat_interval()));
      MessageMeta meta;
      meta.set_cmd(NodeCommand::HEARTBEAT);

      HeartbeatMessage heartbeat_message;
      heartbeat_message.set_node_id(node_info_.node_id_);

      CommMessage message;
      *message.mutable_pb_meta() = {meta};
      message.set_data(heartbeat_message.SerializeAsString());
      SendMessageAsync(client, message);
    }
  });
  heart_beat_thread_->detach();
}

void Node::ProcessHeartbeatResp(const CommMessage &message) {
  HeartbeatRespMessage heartbeat_resp_message;
  heartbeat_resp_message.ParseFromString(message.data());
  is_ready_ = heartbeat_resp_message.is_cluster_ready();
  if (is_ready_.load()) {
    wait_start_cond_.notify_all();
    MS_LOG(DEBUG) << "The node id:" << node_info_.node_id_ << " is ready!";
  }
  is_finish_ = heartbeat_resp_message.is_cluster_finish();
  if (is_finish_.load()) {
    wait_finish_cond_.notify_all();
    MS_LOG(DEBUG) << "The node id:" << node_info_.node_id_ << " is finish!";
  }
  is_timeout_ = heartbeat_resp_message.is_cluster_timeout();
  if (is_timeout_ && on_node_event_message_) {
    is_ready_ = true;
    wait_start_cond_.notify_all();
    on_node_event_message_(NodeEvent::NODE_TIMEOUT);
  }
}

void Node::FetchServers(const std::shared_ptr<TcpClient> &client) {
  MessageMeta meta;
  meta.set_cmd(NodeCommand::FETCH_SERVER);

  CommMessage message;
  *message.mutable_pb_meta() = {meta};
  SendMessageSync(client, message);
}

void Node::ProcessFetchServersResp(const CommMessage &message) {
  FetchServersRespMessage fetch_servers_resp_message;
  fetch_servers_resp_message.ParseFromString(message.data());

  for (const auto &it : fetch_servers_resp_message.servers_meta()) {
    server_rank_ids_[it.rank_id()] = std::make_pair(it.ip(), it.port());
  }

  MS_LOG(DEBUG) << "The all server host size is:" << server_rank_ids_.size();
}

std::string Node::node_id() const { return node_info_.node_id_; }

uint32_t Node::rank_id() const { return node_info_.rank_id_; }

void Node::set_callback(const OnNodeEventMessage &on_node_event_message) {
  on_node_event_message_ = on_node_event_message;
}

void Node::Wait(uint64_t request_id) {
  std::unique_lock<std::mutex> lock(message_mutex_);
  message_tracker_cond_.wait(lock, [&] {
    bool ret = message_tracker_[request_id].first == message_tracker_[request_id].second;
    if (ret) {
      MS_LOG(INFO) << "Message tracker remove request id:" << request_id;
      message_tracker_.erase(request_id);
    }
    return ret;
  });
}

void Node::Send(const enum NodeRole &node_role, const uint32_t &rank_id, CommMessage &message) {
  if (!CommUtil::ValidateRankId(node_role, rank_id)) {
    MS_LOG(ERROR) << "The node role or rank_id is illegal!";
  }

  MessageMeta message_meta;
  message_meta.set_cmd(NodeCommand::SEND_DATA);
  *message.mutable_pb_meta() = {message_meta};

  auto client = GetOrCreateTcpClient(rank_id);
  SendMessageSync(client, message);
}

void Node::Send(const std::vector<std::tuple<const enum NodeRole &, const uint32_t &, const void *, size_t>> &data) {
  uint64_t request_id = ++next_request_id_;
  message_tracker_[request_id] = std::make_pair(data.size(), 0);
  for (auto it = data.begin(); it != data.end(); ++it) {
    NodeRole node_role;
    uint32_t rank_id;
    const void *message;
    size_t len;
    std::tie(node_role, rank_id, message, len) = *it;

    if (!CommUtil::ValidateRankId(node_role, rank_id)) {
      MS_LOG(ERROR) << "The node role or rank_id is illegal!";
    }

    MessageMeta message_meta;
    message_meta.set_cmd(NodeCommand::SEND_DATA);
    message_meta.set_request_id(request_id);

    CommMessage comm_message;
    *comm_message.mutable_pb_meta() = {message_meta};
    comm_message.set_data(message, len);

    auto client = GetOrCreateTcpClient(rank_id);
    client->SendMessage(comm_message);
  }
  Wait(request_id);
}

void Node::Disconnect(const std::shared_ptr<TcpClient> &client) {
  MessageMeta meta;
  meta.set_cmd(NodeCommand::FINISH);

  FinishMessage finish_message;
  finish_message.set_node_id(node_info_.node_id_);

  CommMessage message;
  *message.mutable_pb_meta() = {meta};
  message.set_data(finish_message.SerializeAsString());
  SendMessageSync(client, message);
  MS_LOG(INFO) << "The node id:" << node_info_.node_id_ << " send finish message!";
  WaitForDisconnect();
}

void Node::WaitForStart() {
  std::unique_lock<std::mutex> lock(wait_start_mutex_);
  wait_start_cond_.wait(lock, [&] {
    bool res = is_ready_.load();
    if (res) {
      MS_LOG(INFO) << "The node id:" << node_info_.node_id_ << " is success start!";
    }
    return res;
  });
}

void Node::WaitForDisconnect() {
  std::unique_lock<std::mutex> lock(wait_finish_mutex_);
  wait_finish_cond_.wait(lock, [&] {
    if (is_finish_.load()) {
      MS_LOG(INFO) << "The node id:" << node_info_.node_id_ << " is success finish!";
    }
    return is_finish_.load();
  });
}

void Node::SendMessageSync(const std::shared_ptr<TcpClient> &client, const CommMessage &message) {
  uint64_t request_id = ++next_request_id_;
  message_tracker_[request_id] = std::make_pair(1, 0);
  const_cast<CommMessage &>(message).mutable_pb_meta()->set_request_id(request_id);
  client->SendMessage(message);
  Wait(request_id);
}

void Node::SendMessageAsync(const std::shared_ptr<TcpClient> &client, const CommMessage &message) {
  uint64_t request_id = ++next_request_id_;
  const_cast<CommMessage &>(message).mutable_pb_meta()->set_request_id(request_id);
  client->SendMessage(message);
}

void Node::NotifyMessageArrival(const CommMessage &message) {
  const MessageMeta &message_meta = message.pb_meta();
  uint64_t request_id = message_meta.request_id();

  message_tracker_[request_id].second++;
  message_tracker_cond_.notify_all();
}

const std::shared_ptr<TcpClient> &Node::GetOrCreateTcpClient(const int &rank_id) {
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
          ProcessSendDataResp(message);
          break;
        default:
          MS_LOG(EXCEPTION) << "The cmd:" << message.pb_meta().cmd() << " is not supported!";
      }
      NotifyMessageArrival(message);
    });
    client->Init();
    connected_nodes_[rank_id] = client;
    return connected_nodes_[rank_id];
  }
}

void Node::ProcessSendDataResp(const CommMessage &message) {}
}  // namespace core
}  // namespace ps
}  // namespace mindspore
