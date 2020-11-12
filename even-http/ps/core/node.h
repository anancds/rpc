//
// Created by cds on 2020/10/29.
//

#ifndef RPC_NODE_H
#define RPC_NODE_H

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

// postoffice
class Node {
 public:
  Node()
      : node_id_(0),
        is_system_ready_(false),
        is_system_synchronized_(false),
        current_worker_num_(0),
        current_server_num_(0) {}
  virtual ~Node() = default;

  virtual void Start();
  virtual void Stop();
  void StartHeartBeatTimer(const std::shared_ptr<TcpClient> &client);
  void ProcessHeartbeat(const TcpClient &client, const CommMessage &message);
  void ProcessFetchServers(const CommMessage &message);
  void ProcessRegister(const CommMessage &message);

  uint32_t NodeId();

 protected:
  std::set<std::string> workers_;
  std::set<std::string> servers_;
  std::set<std::string> connected_nodes;
  std::set<std::string> recovery_nodes;

  uint32_t node_id_;
  bool is_system_ready_;
  bool is_system_synchronized_;
  int current_worker_num_;
  int current_server_num_;
  std::unordered_map<std::string, int> connected_nodes_;
};

class ClientNode : public Node {
 public:
  void Start() override;
  void RegisterClient(const std::shared_ptr<TcpClient> &client, const Role &role);
  void Stop() override;

 private:
  std::shared_ptr<TcpClient> client_;
};

class ServerNode : public Node {
 public:
  void Start() override;
  void RegisterServer(const std::shared_ptr<TcpClient> &client, const std::string &host, const uint32_t &port,
                      const Role &role);
  void Stop() override;

 private:
  std::shared_ptr<TcpClient> client_;
  std::shared_ptr<TcpServer> server_;
};

class SchedulerNode : public Node {
 public:
  void Start() override;
  void Stop() override;
  void ProcessHeartBeat(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);
  void ProcessRegister(const TcpServer &server, const TcpConnection &conn, const CommMessage &message);

 private:
  std::unique_ptr<TcpServer> server_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_H
