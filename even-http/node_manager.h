//
// Created by cds on 2020/10/24.
//

#ifndef RPC_NODE_MANAGER_H
#define RPC_NODE_MANAGER_H

namespace mindspore {
namespace ps {
namespace comm {

class NodeManager {
  static NodeManager *Get() {
    static NodeManager e;
    return &e;
  }

  void Start();
};
}  // namespace comm
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_MANAGER_H
