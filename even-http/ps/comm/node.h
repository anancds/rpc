//
// Created by cds on 2020/10/29.
//

#ifndef RPC_NODE_H
#define RPC_NODE_H

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "../../../build/even-http/ps/comm/comm.pb.h"
#include "../../../build/even-http/ps/comm/ps.pb.h"
#include "log_adapter.h"
#include "ps/comm/message.h"
#include "ps/comm/tcp_server.h"

namespace mindspore {
namespace ps {
namespace comm {

// postoffice
class Node {
  virtual void Start();
  virtual void Stop();
};

class ClientNode : public Node {
 public:
  void Start() override;
  void Stop() override;
};

class ServerNode : public Node {
  void Start() override;
  void Stop() override;
};

class SchedulerNode : public Node {
 public:
  void Start() override;
  void Stop() override;

 private:
  std::set<std::string> workers_;
  std::set<std::string> servers_;
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_H
