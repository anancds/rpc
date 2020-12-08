//
// Created by cds on 2020/10/24.
//

#ifndef RPC_NODE_MANAGER_TEST_H
#define RPC_NODE_MANAGER_TEST_H

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ps/core/node.h"
#include "ps/core/scheduler_node.h"
#include "ps/core/worker_node.h"
#include "ps/core/server_node.h"
#include "utils/log_adapter.h"
#include "utils/ms_utils.h"

namespace mindspore {
namespace ps {
namespace core {

class NodeManagerTest {
 public:
  static NodeManagerTest *Get() {
    static NodeManagerTest e;
    return &e;
  }

  NodeManagerTest(const NodeManagerTest &) = delete;
  NodeManagerTest &operator=(const NodeManagerTest &) = delete;
  virtual ~NodeManagerTest() = default;

  void StartScheduler();
  void StartServer();
  void StartClient();

 protected:
  NodeManagerTest() = default;

 private:
  std::unique_ptr<WorkerNode> worker_node_{nullptr};
  std::unique_ptr<SchedulerNode> scheduler_node_{nullptr};
  std::unique_ptr<ServerNode> server_node_{nullptr};
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_MANAGER_TEST_H