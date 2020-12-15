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
#include <string>
#include <vector>
#include <thread>
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
  SchedulerNode() : server_(nullptr), scheduler_thread_(nullptr), state_flush_thread_(nullptr) {}
  ~SchedulerNode() override;

  bool Start(const uint32_t &timeout = kTimeoutInSeconds) override;
  bool Stop() override;
  bool Finish(const uint32_t &timeout = kTimeoutInSeconds) override;

  bool Send(const enum NodeRole &node_role, const uint32_t &rank_id, const std::string &message,
            const uint32_t &timeout = 3) override;
  bool Send(const NodeRole &node_role, const std::vector<uint32_t> &rank_ids, const std::vector<std::string> &data,
            const uint32_t &timeout = 3) override;
  bool Send(const enum NodeRole &node_role, const uint32_t &rank_id, const std::string &message,
            CommMessage *comm_message, const uint32_t &timeout = 3);
  bool Send(const NodeRole &node_role, const std::vector<uint32_t> &rank_ids, const std::vector<std::string> &data,
            std::vector<CommMessage *> *comm_message_resp, const uint32_t &timeout = 3);

 private:
  void Initialize();
  void CreateTcpServer();
  void ProcessHeartBeatCmd(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void ProcessRegisterCmd(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void StartUpdateClusterStateTimer();
  void ProcessFinishCmd(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void ProcessFetchServersCmd(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);

  std::unique_ptr<TcpServer> server_;
  std::unique_ptr<std::thread> scheduler_thread_;
  std::unique_ptr<std::thread> state_flush_thread_;

  NodeManager node_manager_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_SCHEDULER_NODE_H_
