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
#include "ps/core/node_manager.h"
#include "ps/core/node.h"
#include "utils/log_adapter.h"

namespace mindspore {
namespace ps {
namespace core {

class SchedulerNode : public Node {
 public:
  SchedulerNode()
      : server_(nullptr), scheduler_thread_(nullptr),
        state_flush_thread_(nullptr), cluster_available_thread_(nullptr) {}
  ~SchedulerNode() override;

  void Start() override;
  void Stop() override;
  void Finish() override;

 private:
  void InitNode();
  void Init();
  void ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void StartClusterStateFlushTimer();
  void ProcessFinish(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void ProcessFetchServers(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);

  std::unique_ptr<TcpServer> server_;
  std::unique_ptr<std::thread> scheduler_thread_;
  std::unique_ptr<std::thread> state_flush_thread_;
  std::unique_ptr<std::thread> cluster_available_thread_;

  NodeManager node_manager_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_SCHEDULER_NODE_H_
