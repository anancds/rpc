//
// Created by cds on 2020/10/24.
//

#ifndef RPC_NODE_MANAGER_H
#define RPC_NODE_MANAGER_H

#include <string>

#include "network_manager.h"
#include "utils/log_adapter.h"
#include "utils/ms_utils.h"

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

  void Init();

  uint32_t num_workers() const;
  uint32_t num_servers() const;
  std::string node_role() const;

 private:
  uint32_t num_servers_;
  uint32_t num_workers_;
  std::string role_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_MANAGER_H
