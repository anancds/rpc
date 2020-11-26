//
// Created by cds on 2020/10/24.
//

#include "node_manager.h"

namespace mindspore {
namespace ps {
namespace core {

void NodeManager::StartScheduler() {
  scheduler_node_ = std::make_unique<SchedulerNode>();
  scheduler_node_->set_callback([&](const NodeEvent &event){
    if (event == NodeEvent::CLUSTER_TIMEOUT) {
      scheduler_node_->Stop();
    }
  });
  scheduler_node_->Start();
//  while (true) {
//    std::this_thread::sleep_for(std::chrono::milliseconds(50));
//  }
}

void NodeManager::StopScheduler() { scheduler_node_->Stop(); }

void NodeManager::StartServer() {
  server_node_ = std::make_unique<ServerNode>();
  server_node_->set_callback([&](const NodeEvent &event){
    if (event == NodeEvent::TERMINATE_READY) {
      server_node_->Stop();
    }
  });
  server_node_->Start();
}

void NodeManager::StopServer() { server_node_->Stop(); }

void NodeManager::StartClient() {
  worker_node_ = std::make_unique<WorkerNode>();
  worker_node_->set_callback([&](const NodeEvent &event){
        if (event == NodeEvent::TERMINATE_READY) {
          worker_node_->Stop();
        }
       });
  worker_node_->Start();
  worker_node_->Finish();
  worker_node_->Stop();
}

void NodeManager::StopClient() { worker_node_->Stop(); }

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