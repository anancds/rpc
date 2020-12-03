/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "common_test.h"
#include "ps/core/node.h"
#include "ps/core/scheduler_node.h"
#include "ps/core/worker_node.h"
#include "ps/core/server_node.h"
#include "ps/core/tcp_client.h"
#include "ps/core/tcp_server.h"

#include <memory>
#include <thread>

namespace mindspore {
namespace ps {
namespace core {
class TestClusterConnection : public UT::Common {
 public:
  TestClusterConnection() : scheduler_node_(nullptr), server_node_(nullptr), client_node_(nullptr) {}
  ~TestClusterConnection() override = default;

  void SetUp() override {
    ClusterConfig::Init(1, 1, std::make_unique<std::string>("127.0.0.1"), 9999);
    scheduler_node_ = std::make_unique<SchedulerNode>();
    std::unique_ptr<std::thread> scheduler_server_thread_(nullptr);
    scheduler_server_thread_ = std::make_unique<std::thread>([&]() { scheduler_node_->Start(); });
    scheduler_server_thread_->detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  }
  void TearDown() override {
  }

  std::unique_ptr<Node> scheduler_node_;
  std::unique_ptr<Node> server_node_;
  std::unique_ptr<Node> client_node_;
};

TEST_F(TestClusterConnection, StartServerAndClient) {
  server_node_ = std::make_unique<ServerNode>();
  std::unique_ptr<std::thread> server_thread_(nullptr);
  server_thread_ = std::make_unique<std::thread>([&]() { server_node_->Start(); });
  server_thread_->detach();

  client_node_ = std::make_unique<WorkerNode>();
  std::unique_ptr<std::thread> client_server_thread_(nullptr);
  client_server_thread_ = std::make_unique<std::thread>([&]() { client_node_->Start(); });
  client_server_thread_->detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore