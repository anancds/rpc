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

  client->set_timer_callback([&](const TcpClient &client) {
    MessageMeta meta;
    meta.set_cmd(NodeCommand::HEARTBEAT);
    meta.set_request_id(++request_id_);

    HeartbeatMessage heartbeat_message;
    heartbeat_message.set_node_id(node_info_.node_id_);

    CommMessage message;
    *message.mutable_pb_meta() = {meta};
    message.set_data(heartbeat_message.SerializeAsString());
    client.SendMessage(message);
  });
  client->StartTimer(ClusterConfig::heartbeat_interval());
}

void Node::ProcessHeartbeat(const CommMessage &message) {
  HeartbeatRespMessage heartbeat_resp_message;
  heartbeat_resp_message.ParseFromString(message.data());
  is_cluster_ready_ = heartbeat_resp_message.is_cluster_ready();
  bool is_cluster_finish = heartbeat_resp_message.is_cluster_finish();
  if (is_cluster_finish) {
    is_cluster_finish_ = true;
    message_tracker_cond_.notify_all();
  }
  bool is_node_timeout = heartbeat_resp_message.is_node_timeout();
  if (is_node_timeout && on_node_event_message_) {
    on_node_event_message_(NodeEvent::NODE_TIMEOUT);
  }
}

uint64_t Node::FetchServers(const std::shared_ptr<TcpClient> &client) {
  MessageMeta meta;
  meta.set_cmd(NodeCommand::FETCH_SERVER);
  uint64_t request_id = AssignRequestId(1);
  meta.set_request_id(request_id);

  CommMessage message;
  *message.mutable_pb_meta() = {meta};
  client->SendMessage(message);
  return request_id;
}

void Node::ProcessFetchServers(const CommMessage &message) {
  const MessageMeta &message_meta = message.pb_meta();
  uint64_t request_id = message_meta.request_id();

  FetchServersRespMessage fetch_servers_resp_message;
  fetch_servers_resp_message.ParseFromString(message.data());

  auto meta_begin = fetch_servers_resp_message.servers_meta().begin();
  auto meta_end = fetch_servers_resp_message.servers_meta().end();
  for (auto it = meta_begin; it != meta_end; ++it) {
    server_rank_ids_[it->rank_id()] = std::make_pair(it->ip(), it->port());
  }

  message_tracker_[request_id].second++;
  message_tracker_cond_.notify_all();
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
      MS_LOG(INFO) << "Message tracker remove request id!";
      message_tracker_.erase(request_id);
    }
    return ret;
  });
}

uint64_t Node::AssignRequestId(const uint32_t &expected_resp_num) {
  std::lock_guard<std::mutex> lock(message_mutex_);
  uint64_t request_id = ++request_id_;
  message_tracker_[request_id] = std::make_pair(expected_resp_num, 0);
  return request_id;
}

}  // namespace core
}  // namespace ps
}  // namespace mindspore
