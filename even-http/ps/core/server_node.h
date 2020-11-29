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

#ifndef MINDSPORE_CCSRC_PS_CORE_SERVER_NODE_H_
#define MINDSPORE_CCSRC_PS_CORE_SERVER_NODE_H_

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
class ServerNode : public Node {
 public:
  ServerNode()
      : client_to_scheduler_(nullptr),
        server_(nullptr),
        client_to_scheduler_thread_(nullptr),
        server_thread_(nullptr) {}
  ~ServerNode() override;

  void Start() override;
  void Stop() override;
  void Finish() override;

  using RequestHandler =
    std::function<void(const TcpServer &server, const TcpConnection &conn, const CommMessage &message)>;

  void Send(const enum NodeRole &node_role, uint32_t rank_id, const CommMessage &message);
  void set_handler(const RequestHandler &handler);
  void Response(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);

 private:
  void Register(const std::shared_ptr<TcpClient> &client);
  void ProcessRegister(const CommMessage &message);
  const std::shared_ptr<TcpClient> &GetOrCreateTcpClient(const int &rank_id);
  void Init();
  void InitNode();
  void InitClientToScheduler();
  void ProcessSendData(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);

  std::shared_ptr<TcpClient> client_to_scheduler_;
  std::shared_ptr<TcpServer> server_;
  std::unique_ptr<std::thread> client_to_scheduler_thread_;
  std::unique_ptr<std::thread> server_thread_;
  // rank_id->tcpclient
  std::unordered_map<int, std::shared_ptr<TcpClient>> connected_nodes_;
  std::mutex client_mutex_;
  RequestHandler request_handler_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_SERVER_NODE_H_
