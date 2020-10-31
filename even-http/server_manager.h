//
// Created by cds on 2020/10/24.
//

#ifndef RPC_SERVER_MANAGER_H
#define RPC_SERVER_MANAGER_H

#include <functional>
#include <iostream>
#include "message.h"

namespace mindspore {
namespace ps {
namespace comm {

template <typename Val>
class Server {
 public:
  using ReqHandle = std::function<void(
    const Message &req_meta, const std::pair<std::vector<Val>, std::vector<Val>> &kvs, Server *server, void *dest)>;
  void SetCallBack(const ReqHandle &handle);
  void Response();
  void Process(const Message &message);
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_SERVER_MANAGER_H
