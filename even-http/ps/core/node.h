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

#ifndef RPC_NODE_H
#define RPC_NODE_H

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "ps/core/cluster_config.h"
#include "ps/core/tcp_client.h"
#include "ps/core/tcp_server.h"
#include "utils/log_adapter.h"

namespace mindspore {
namespace ps {
namespace core {

constexpr uint32_t kSchedulerNodeId = 0;

class Node {
 public:
  Node() : node_id_(0), is_system_ready_(false), current_worker_rank_id_(-1), current_server_rank_id_(-1) {}
  virtual ~Node() = default;

  virtual void Start() = 0;
  virtual void Stop() = 0;

  uint32_t NodeId() const;
  int RankId() const;

 protected:
  void Heartbeat(const std::shared_ptr<TcpClient> &client);
  void ProcessHeartbeat();
  void UpdateHeartbeat(const uint32_t &node_id, const timeval &time);

  uint32_t node_id_;
  bool is_system_ready_;
  std::atomic<int> current_worker_rank_id_;
  std::atomic<int> current_server_rank_id_;
  std::unordered_map<uint32_t, std::shared_ptr<TcpClient>> connected_nodes_;
  std::unordered_map<uint32_t, std::string> server_node_ids_;
  std::unordered_map<uint32_t, timeval> heartbeats_;
  std::mutex heartbeat_mutex_;
};

class ClientNode : public Node {
 public:
  ClientNode() : client_to_scheduler_(nullptr) {}
  ~ClientNode() override = default;
  void Start() override;
  void Stop() override;
  void Send(const enum NodeRole &node_role, uint32_t rank_id, const CommMessage &message);

 private:
  void Register(const std::shared_ptr<TcpClient> &client, const NodeRole &role);
  void ProcessRegister(const CommMessage &message);
  void ProcessTerminal(const CommMessage &message);

  std::shared_ptr<TcpClient> client_to_scheduler_;
  std::unordered_map<int, const TcpClient &> clients_to_servers_;
};

class ServerNode : public Node {
 public:
  ServerNode() : client_to_scheduler_(nullptr), server_(nullptr) {}
  ~ServerNode() override = default;

  void Start() override;
  void Stop() override;

 private:
  void Register(const std::shared_ptr<TcpClient> &client, const std::string &host, const uint32_t &port,
                const NodeRole &role);
  void ProcessRegister(const CommMessage &message);
  void ProcessTerminal(const CommMessage &message);
  std::shared_ptr<TcpClient> client_to_scheduler_;
  std::shared_ptr<TcpServer> server_;
};

class SchedulerNode : public Node {
 public:
  SchedulerNode() : server_(nullptr) {}
  ~SchedulerNode() override = default;

  void Start() override;
  void Stop() override;

 private:
  void HeartBeat(const TcpServer &server, const TcpConnection &conn);
  void ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void Terminal(const TcpServer &server);
  std::unique_ptr<TcpServer> server_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_H
