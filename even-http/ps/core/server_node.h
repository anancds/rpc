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

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "ps/core/cluster_config.h"
#include "ps/core/tcp_client.h"
#include "ps/core/tcp_server.h"
#include "ps/core/abstract_node.h"
#include "utils/log_adapter.h"

namespace mindspore {
namespace ps {
namespace core {
class ServerNode : public AbstractNode {
 public:
  ServerNode() : server_(nullptr), server_thread_(nullptr) {}
  ~ServerNode() override;

  bool Start(const uint32_t &timeout = kTimeoutInSeconds) override;
  bool Stop() override;
  bool Finish(const uint32_t &timeout = kTimeoutInSeconds) override;

  using RequestHandler = std::function<void(const TcpServer &server, const TcpConnection &conn,
                                            const MessageMeta message_meta, const std::string &message)>;

  void set_handler(const RequestHandler &handler);
  void Response(const TcpServer &server, const TcpConnection &conn, const MessageMeta &message_meta,
                const std::string &message);
  uint32_t CollReceive(const uint32_t &rank_id, CommMessage *comm_message_resp);
  bool CollWaitFor(const uint32_t &rank_id, const uint32_t &timeout = kCommTimeoutInSeconds);

 private:
  void CreateTcpServer();
  void Initialize();
  void ProcessSendData(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void set_received_data_callback(const uint32_t &rank_id, const MessageCallback &received_data_callbacks);
  void RunReceivedDataCallback(const uint32_t &rank_id);

  std::shared_ptr<TcpServer> server_;
  std::unique_ptr<std::thread> server_thread_;
  RequestHandler request_handler_;
  std::unordered_map<uint32_t, CommMessage> received_data_;
  std::mutex received_data_callbacks_mutex_;
  std::unordered_map<uint64_t, MessageCallback> received_data_callbacks_;
  std::condition_variable received_data_cond_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // MINDSPORE_CCSRC_PS_CORE_SERVER_NODE_H_
