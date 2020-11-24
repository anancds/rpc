//
// Created by cds on 2020/10/24.
//

#include "node_manager.h"

namespace mindspore {
namespace ps {
namespace core {

void NodeManager::StartScheduler() {
  node_ = std::make_unique<SchedulerNode>();
  node_->set_callback([&](const NodeEvent &event){
    if (event == NodeEvent::CLUSTER_TIMEOUT) {
      node_->Stop();
    }
  });
  node_->Start();
//  while (true) {
//    std::this_thread::sleep_for(std::chrono::milliseconds(50));
//  }
}

void NodeManager::StopScheduler() { node_->Stop(); }

void NodeManager::StartServer() {
  node_ = std::make_unique<ServerNode>();
  node_->Start();
}

void NodeManager::StopServer() { node_->Stop(); }

void NodeManager::StartClient() {
  node_ = std::make_unique<WorkerNode>();
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


}  // namespace core
}  // namespace ps

}  // namespace mindspore