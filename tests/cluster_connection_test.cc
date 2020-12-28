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

#include "common/common_test.h"
#include "ps/core/node.h"
#include "ps/core/scheduler_node.h"
#include "ps/core/worker_node.h"
#include "ps/core/server_node.h"
#include "ps/core/tcp_server.h"

#include <memory>
#include <thread>

namespace mindspore {
namespace ps {
namespace core {
class TestClusterConnection : public UT::Common {
 public:
  TestClusterConnection()
      : scheduler_node_(nullptr),
        server_node_(nullptr),
        worker_node_(nullptr),
        scheduler_thread_(nullptr),
        server_thread_(nullptr),
        worker_thread_(nullptr) {}
  ~TestClusterConnection() override = default;

  void SetUp() override {
    ClusterConfig::Init(1, 1, std::make_unique<std::string>("127.0.0.1"), 9999);
    scheduler_node_ = std::make_unique<SchedulerNode>();
    scheduler_thread_ = std::make_unique<std::thread>([&]() {
      scheduler_node_->Start();
      scheduler_node_->Finish();
      scheduler_node_->Stop();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  }
  void TearDown() override {
    scheduler_thread_->join();
    worker_thread_->join();
    server_thread_->join();
  }

  std::unique_ptr<SchedulerNode> scheduler_node_;
  std::unique_ptr<ServerNode> server_node_;
  std::unique_ptr<WorkerNode> worker_node_;

  std::unique_ptr<std::thread> scheduler_thread_;
  std::unique_ptr<std::thread> server_thread_;
  std::unique_ptr<std::thread> worker_thread_;
};

TEST_F(TestClusterConnection, StartServerAndClient) {
  server_node_ = std::make_unique<ServerNode>();
  server_thread_ = std::make_unique<std::thread>([&]() {
    server_node_->set_handler(
      [&](const TcpServer &server, const TcpConnection &conn, const MessageMeta &message_meta,
          const std::string &message) { server_node_->Response(server, conn, message_meta, message); });
    server_node_->Start();
    server_node_->Finish(3);
    server_node_->Stop();
  });

  worker_node_ = std::make_unique<WorkerNode>();
  worker_thread_ = std::make_unique<std::thread>([&]() {
    worker_node_->Start();
    worker_node_->Finish(3);
    worker_node_->Stop();
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore