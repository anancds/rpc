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

#include "../../../build/even-http/ps/comm/comm.pb.h"
#include "../../../build/even-http/ps/comm/ps.pb.h"
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
  virtual void Start();
  virtual void Stop();
  void StartHeartBeatTimer(TcpClient &client);
  void ProcessAck(const TcpClient &client, const CommMessage &message);
  void ProcessNode(const CommMessage &message);

 protected:
  std::set<std::string> workers_;
  std::set<std::string> servers_;
  std::set<std::string> connected_nodes;
  std::set<std::string> recovery_nodes;

 private:
  bool node_id_;
  bool is_system_ready_;
  bool is_system_synchronized_;
};

class ClientNode : public Node {
 public:
  void Start() override;
  void RegisterClient(const TcpClient &client, const Role &role);
  void Stop() override;
};

class ServerNode : public Node {
 public:
  void Start() override;
  void RegisterServer(const TcpClient &client, const std::string &host, const uint32_t &port, const Role &role);
  void Stop() override;

 private:
  std::unique_ptr<TcpClient> client_;
  std::unique_ptr<TcpServer> server_;
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
