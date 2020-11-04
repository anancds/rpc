//
// Created by cds on 2020/10/24.
//

#ifndef RPC_NODE_MANAGER_H
#define RPC_NODE_MANAGER_H

#include "network_manager.h"

namespace mindspore {
namespace ps {
namespace comm {
//相当于postoffice

class NodeManager {
 public:
  static NodeManager *Get() {
    static NodeManager e;
    return &e;
  }

  void Start();

  void Stop();

 private:
  NetWorkManager *network_manager_;
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_MANAGER_H
