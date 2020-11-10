//
// Created by cds on 2020/10/24.
//

#include "node_manager.h"

namespace mindspore {
namespace ps {
namespace comm {

void NodeManager::Init() {
  std::string num_workers = common::GetEnv("MS_WORKER_NUM");
  if (num_workers.empty()) {
    MS_LOG(EXCEPTION) << "The MS_WORKER_NUM should not be null!";
  }
  num_workers_ = strtol(num_workers.c_str(), nullptr, 10);

  std::string num_servers = common::GetEnv("MS_SERVER_NUM");
  if (num_servers.empty()) {
    MS_LOG(EXCEPTION) << "The MS_SERVER_NUM should not be null!";
  }
  num_servers_ = strtol(num_servers.c_str(), nullptr, 10);

  std::string role = common::GetEnv("MS_ROLE");
  if (role.empty()) {
    MS_LOG(EXCEPTION) << "The MS_ROLE should not be null!";
  }
}

uint32_t NodeManager::num_workers() const { return num_workers_; }

uint32_t NodeManager::num_servers() const { return num_servers_; }

std::string NodeManager::node_role() const { return role_; }

}  // namespace core
}  // namespace ps

}  // namespace mindspore