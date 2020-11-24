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

#ifndef MINDSPORE_CCSRC_PS_CORE_CLIENT_NODE_H_
#define MINDSPORE_CCSRC_PS_CORE_CLIENT_NODE_H_

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
#include <utility>
#include <condition_variable>

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "ps/core/cluster_config.h"
#include "ps/core/tcp_client.h"
#include "ps/core/tcp_server.h"
#include "ps/core/node.h"
#include "utils/log_adapter.h"

namespace mindspore {
namespace ps {
namespace core {

class WorkerNode : public Node {
 public:
  WorkerNode() : client_to_scheduler_(nullptr), worker_thread_(nullptr), timestamp_(0) {}
  ~WorkerNode() override = default;

  void Start() override;
  void Stop() override;

  void Send(const enum NodeRole &node_role, uint32_t rank_id, CommMessage &message);
  void Wait(uint64_t timestamp);
  void SendForData();

 private:
  void Register(const std::shared_ptr<TcpClient> &client, const NodeRole &role);
  void ProcessRegister(const CommMessage &message);
  void ProcessTerminal(const CommMessage &message);
  void ProcessData(const CommMessage &message);
  const std::shared_ptr<TcpClient> &GetOrCreateTcpClient(const int &rank_id);
  uint64_t AssignTimestamp(const uint32_t &server_sent_num);

  std::shared_ptr<TcpClient> client_to_scheduler_;
  std::unique_ptr<std::thread> worker_thread_;
  std::atomic_uint64_t timestamp_;
  // rank_id-><node_id, ip:port>
  std::unordered_map<int, std::pair<std::string, std::string>> server_node_rank_ids_;
  // rank_id->tcpclient
  std::unordered_map<int, std::shared_ptr<TcpClient>> connected_nodes_;
  std::mutex client_mutex_;
  // timestamp-><expected responses, actual responses>
  std::unordered_map<uint64_t, std::pair<uint32_t, uint32_t>> message_tracker_;
  std::mutex message_mutex_;
  std::condition_variable message_tracker_cond_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_CLIENT_NODE_H_
