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

#ifndef MINDSPORE_CCSRC_PS_CORE_NODE_H_
#define MINDSPORE_CCSRC_PS_CORE_NODE_H_

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <thread>
#include <map>
#include <unordered_map>
#include <functional>

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "ps/core/cluster_config.h"
#include "ps/core/tcp_client.h"
#include "ps/core/tcp_server.h"
#include "utils/log_adapter.h"

namespace mindspore {
namespace ps {
namespace core {

class Node {
 public:
  Node() : rank_id_(0), is_cluster_ready_(false), is_node_stop_(true) {}
  virtual ~Node() = default;

  using OnNodeEventMessage = std::function<void(const NodeEvent &event)>;

  virtual void Start() = 0;
  virtual void Stop() = 0;
  void set_callback(const OnNodeEventMessage &on_node_event_message);

  std::string node_id() const;
  uint32_t rank_id() const;

 protected:
  void Heartbeat(const std::shared_ptr<TcpClient> &client) const;
  void ProcessHeartbeat(const CommMessage &message);
  void UpdateHeartbeat(const std::string &node_id, const timeval &time);

  std::string node_id_;
  uint32_t rank_id_;
  NodeRole node_role_;
  std::atomic<bool> is_cluster_ready_;
  std::atomic<bool> is_node_stop_;

  std::unordered_map<std::string, timeval> heartbeats_;
  std::mutex heartbeat_mutex_;
  OnNodeEventMessage on_node_event_message_;
};

}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_NODE_H_
