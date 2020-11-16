//
// Created by cds on 2020/10/24.
//

#include "node_manager.h"

namespace mindspore {
namespace ps {
namespace core {

void NodeManager::StartScheduler() {
  node_ = std::make_unique<SchedulerNode>();
  node_->Start();
}

void NodeManager::StopScheduler() { node_->Stop(); }

void NodeManager::StartServer() {
  node_ = std::make_unique<ServerNode>();
  node_->Start();
}

void NodeManager::StopServer() { node_->Stop(); }

void NodeManager::StartClient() {
  node_ = std::make_unique<ClientNode>();
  node_->Start();
}

void NodeManager::StopClient() { node_->Stop(); }

int NodeManager::WorkerRankToID(int rank) { return rank * 2 + 9; }

int NodeManager::ServerRankToID(int rank) { return rank * 2 + 8; }

const std::vector<int> &NodeManager::GetNodeIDs(int node_id) const {
  const auto it = node_ids_.find(node_id);
  return it->second;
}

int NodeManager::IDtoRank(int id) { return std::max((id - 8) / 2, 0); }

int NodeManager::my_rank() const { return IDtoRank(node_->NodeId()); }

}  // namespace core
}  // namespace ps

}  // namespace mindspore