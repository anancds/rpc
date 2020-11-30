//
// Created by cds on 2020/11/30.
//

#ifndef RPC_CLUSTER_MANAGER_H
#define RPC_CLUSTER_MANAGER_H

#include <atomic>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <condition_variable>
#include <unordered_set>

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "ps/core/node.h"
#include "utils/log_adapter.h"

namespace mindspore {
namespace ps {
namespace core {
class NodeManager {
 public:
  NodeManager()
      : is_cluster_ready_(false),
        is_cluster_finish_(false),
        is_cluster_timeout_(false),
        total_node_num_(0),
        current_worker_rank_id_(-1),
        current_server_rank_id_(-1) {}
  virtual ~NodeManager() = default;

  void InitNodesNum();
  int NextRankId(const RegisterMessage &register_message);
  void UpdateHeartbeat(const std::string &node_id);
  std::vector<ServersMeta> FetchServersMeta();
  void ClusterStateFlush();
  void ClusterAvailableFlush();
  void FinishNodesStateFlush(const FinishMessage& finish_message);
  std::unordered_map<std::string, NodeInfo> nodes_info();
  bool is_cluster_ready();
  bool is_cluster_finish();
  bool is_cluster_timeout();

 private:
  std::atomic<bool> is_cluster_ready_;
  std::atomic<bool> is_cluster_finish_;
  std::atomic<bool> is_cluster_timeout_;
  uint32_t total_node_num_;
  std::atomic<int> current_worker_rank_id_;
  std::atomic<int> current_server_rank_id_;
  // worker nodes and server nodes
  std::unordered_map<std::string, NodeInfo> nodes_info_;
  std::mutex assign_rank_id_mutex_;
  std::mutex heartbeat_mutex_;
  std::unordered_map<std::string, timeval> heartbeats_;
  // timeout nodes
  std::unordered_map<std::string, NodeInfo> timeout_nodes_info_;
  std::unordered_set<std::string> finish_nodes_id_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_CLUSTER_MANAGER_H
