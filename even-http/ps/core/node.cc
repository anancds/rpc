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

void Node::Heartbeat(const std::shared_ptr<TcpClient> &client) const {
  MS_LOG(INFO) << "The node role: " << node_role_ << ", the node id:" << node_id_ << ", the node rank id:" << rank_id_
               << " begin send heartbeat to the scheduler!";
  if (on_node_event_message_) {
    on_node_event_message_(NodeEvent::HEARTBEAT_START);
  }

  client->set_timer_callback([&](const TcpClient &client) {
    MessageMeta meta;
    meta.set_cmd(ClusterCommand::HEARTBEAT);
    meta.set_node_id(node_id_);
    CommMessage message;
    *message.mutable_pb_meta() = {meta};
    client.SendMessage(message);
  });
  client->StartTimer(ClusterConfig::heartbeat_interval());
}

void Node::ProcessHeartbeat(const CommMessage &message) {
  RegisterMessage register_message;
  register_message.ParseFromString(message.data());
  struct timeval current_time {};
  (void)gettimeofday(&current_time, nullptr);
  UpdateHeartbeat(register_message.node_id(), current_time);
}

void Node::UpdateHeartbeat(const std::string &node_id, const timeval &time) {
  std::lock_guard<std::mutex> lock(heartbeat_mutex_);

  heartbeats_[node_id] = time;
  MS_LOG(INFO) << "The node role: " << node_role_ << ", the node id:" << node_id_ << ", the node rank id:" << rank_id_
               << " the current time is: " << time.tv_sec;
}

std::string Node::node_id() const { return node_id_; }

uint32_t Node::rank_id() const { return rank_id_; }

void Node::set_callback(const OnNodeEventMessage &on_node_event_message) {
  on_node_event_message_ = on_node_event_message;
}

}  // namespace core
}  // namespace ps
}  // namespace mindspore
