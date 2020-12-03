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
#include <string>
#include <vector>
#include <thread>
#include <unordered_map>
#include <utility>
#include <condition_variable>
#include <tuple>

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
  WorkerNode() : client_to_scheduler_(nullptr), worker_thread_(nullptr) {}
  ~WorkerNode() override;

  void Start() override;
  void Stop() override;
  void Finish() override;

  void Send(const enum NodeRole &node_role, const uint32_t &rank_id, CommMessage &message);
  void Send(const std::vector<std::tuple<const enum NodeRole &, const uint32_t &, CommMessage &>> &data);
  void BroadCast(CommMessage &message);

 private:
  void Register();
  void ProcessRegisterResp(const CommMessage &message);
  void ProcessData(const CommMessage &message);
  const std::shared_ptr<TcpClient> &GetOrCreateTcpClient(const int &rank_id);

  void InitNode();
  void InitClientToScheduler();


  std::shared_ptr<TcpClient> client_to_scheduler_;
  std::unique_ptr<std::thread> worker_thread_;

  // rank_id->tcpclient
  std::unordered_map<int, std::shared_ptr<TcpClient>> connected_nodes_;
  std::mutex client_mutex_;

};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_CLIENT_NODE_H_
