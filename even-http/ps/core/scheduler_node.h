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

#ifndef MINDSPORE_CCSRC_PS_CORE_SCHEDULER_NODE_H_
#define MINDSPORE_CCSRC_PS_CORE_SCHEDULER_NODE_H_

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
#include <mutex>

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

class SchedulerNode : public Node {
 public:
  SchedulerNode()
      : server_(nullptr), current_worker_rank_id_(-1), current_server_rank_id_(-1), scheduler_thread_(nullptr) {}
  ~SchedulerNode() override;

  void Start() override;
  void Stop() override;

 private:
  void InitNode();
  void Init();
  void UpdateHeartbeat(const std::string &node_id);
  void ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void StartClusterAvailableTimer();
  void StartHeartbeatTimer();
  int AssignRankId(const RegisterMessage &register_message);
  void ProcessFinish(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void ProcessFetchServers(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);

  std::unique_ptr<TcpServer> server_;
  std::atomic<int> current_worker_rank_id_;
  std::atomic<int> current_server_rank_id_;
  std::unique_ptr<std::thread> scheduler_thread_;
  // worker nodes and server nodes
  std::unordered_map<std::string, NodeInfo> nodes_info_;
  // timeout nodes
  std::unordered_map<std::string, NodeInfo> timeout_nodes_info_;
  std::mutex assign_rank_id_mutex_;
  std::mutex heartbeat_mutex_;
  std::unordered_set<std::string> finish_nodes_id_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_SCHEDULER_NODE_H_
