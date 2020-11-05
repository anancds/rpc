//
// Created by cds on 2020/10/24.
//

#ifndef RPC_WORKER_MANAGER_H
#define RPC_WORKER_MANAGER_H

#include <functional>
#include <iostream>
#include <vector>
#include "../../../build/even-http/ps/comm/comm.pb.h"
#include "message.h"

namespace mindspore {
namespace ps {
namespace comm {

class Client {
 public:
  using Callback = std::function<void()>;
  //发送接受的是pb的数据

  template <typename Val>
  int Send(const std::vector<uint64_t> &keys, const std::vector<Val> &vals, const std::vector<int> &lens = {},
             int cmd = 0, const Callback &cb = nullptr);

  template <typename Val>
  int Receive(const std::vector<uint64_t> &keys, std::vector<Val> *vals, std::vector<int> *lens = nullptr, int cmd = 0,
             const Callback &cb = nullptr, int priority = 0);
//  int PushString(const std::string &values, const std::vector<int> &lens = {}, int cmd = 0,
//                 const Callback &cb = nullptr);
//
//  int PullString(const std::string *values, int cmd = 0, const Callback &cb = nullptr, int priority = 0);

  void Wait(int timestamp);

  template <typename Val>
  void Send(int timestamp, bool push, bool pull, int cmd, const std::pair<std::vector<Val>, std::vector<Val>> &kvs);

  void Process(const CommMessage &msg);
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_WORKER_MANAGER_H
