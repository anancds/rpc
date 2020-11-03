//
// Created by cds on 2020/10/29.
//

#ifndef RPC_NODE_H
#define RPC_NODE_H

#include <stdlib.h>
#include <vector>
#include <iostream>
#include <string>

#include "log_adapter.h"
#include "tcp_server.h"
#include "../build/even-http/comm.pb.h"


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
  std::vector<std::string> worker_ips_;
  std::vector<std::string> server_ips_;
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_H
