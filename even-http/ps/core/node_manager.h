//
// Created by cds on 2020/10/24.
//

#ifndef RPC_NODE_MANAGER_H
#define RPC_NODE_MANAGER_H

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "network_manager.h"
#include "ps/core/node.h"
#include "ps/core/scheduler_node.h"
#include "ps/core/worker_node.h"
#include "ps/core/server_node.h"
#include "utils/log_adapter.h"
#include "utils/ms_utils.h"

namespace mindspore {
namespace ps {
namespace core {
//这个作为node的成员变量，或者工具类,去掉start和stop

class NodeManager {
 public:
  static NodeManager *Get() {
    static NodeManager e;
    return &e;
  }

  NodeManager(const NodeManager &) = delete;
  NodeManager &operator=(const NodeManager &) = delete;
  virtual ~NodeManager() = default;

  void StartScheduler();
  void StopScheduler();
  void StartServer();
  void StopServer();
  void StartClient();
  void StopClient();

  // 下面的函数是不是应该放到commutil，否则和node.h相互依赖了
  static int WorkerRankToID(int rank);
  static int ServerRankToID(int rank);
  const std::vector<int> &GetNodeIDs(int node_id) const;
  static int IDtoRank(int id);
  int my_rank() const;

 protected:
  NodeManager() = default;

 private:
  std::unordered_map<int, std::vector<int>> node_ids_;
  std::unique_ptr<WorkerNode> worker_node_{nullptr};
  std::unique_ptr<SchedulerNode> scheduler_node_{nullptr};
  std::unique_ptr<ServerNode> server_node_{nullptr};
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore

#endif  // RPC_NODE_MANAGER_H
