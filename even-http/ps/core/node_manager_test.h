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
  void StartServer1();
  void StartClient();

  void ThreadResponse(std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> meesage);

  void CollSend(const uint32_t &rank_id);

 protected:
  NodeManagerTest() = default;
  void CollectiveTest(const uint32_t &rank_id);
  void BroadCastTest();
  void PushTest(const uint32_t &size);
  void PullTest(const uint32_t &size);
  void MultiPullTest(const uint32_t &size);
  void PackKVMessage(std::shared_ptr<CommMessage> message);

 private:
  std::shared_ptr<WorkerNode> worker_node_{nullptr};
  std::shared_ptr<SchedulerNode> scheduler_node_{nullptr};
  std::shared_ptr<ServerNode> server_node_{nullptr};
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_MANAGER_TEST_H
