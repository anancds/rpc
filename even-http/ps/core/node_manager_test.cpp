//
// Created by cds on 2020/10/24.
//

#include "node_manager_test.h"

namespace mindspore {
namespace ps {
namespace core {

void NodeManagerTest::StartScheduler() {
  scheduler_node_ = std::make_unique<SchedulerNode>();
  //  scheduler_node_->set_callback([&](const NodeEvent &event){
  //    if (event == NodeEvent::NODE_TIMEOUT) {
  //      scheduler_node_->Finish();
  //      scheduler_node_->Stop();
  //    }
  //  });
  scheduler_node_->Start();
  scheduler_node_->Finish();
  scheduler_node_->Stop();
  //  while (true) {
  //    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  //  }
}

void NodeManagerTest::StartServer() {
  server_node_ = std::make_unique<ServerNode>();
  server_node_->set_callback([&](const NodeEvent &event) {
    if (event == NodeEvent::NODE_TIMEOUT) {
      MS_LOG(INFO) << "111111111111111111111111111111!";
      //      server_node_->Finish();
      //      server_node_->Stop();
    }
  });
  server_node_->set_handler([&](const TcpServer &server, const TcpConnection &conn, const MessageMeta &message_meta,
                                const std::string &message) {
    KVMessage kv_message;
    //    kv_message.ParseFromString(message.data());
    //    MS_LOG(INFO) << "size:" << kv_message.keys_size();

    server_node_->Response(server, conn, message_meta, message);
  });
  server_node_->Start();

  server_node_->Finish();
  server_node_->Stop();
}

void NodeManagerTest::StartClient() {
  worker_node_ = std::make_unique<WorkerNode>();
  worker_node_->set_callback([&](const NodeEvent &event) {
    if (event == NodeEvent::NODE_TIMEOUT) {
      MS_LOG(INFO) << "NODE_TIMEOUT, finish!";
      std::this_thread::sleep_for(std::chrono::milliseconds(50000));
      //      worker_node_->Finish();
      //      worker_node_->Stop();
    }
  });
  worker_node_->Start();
  KVMessage kv_message;
  std::vector<int> keys(100000000, 1);
  std::vector<int> values(100000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};

  auto start1 = std::chrono::high_resolution_clock::now();
  const auto &message = kv_message.SerializeAsString();
  auto end1 = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "serialize, cost:" << (end1 - start1).count() / 1e6 << "ms";

  auto start = std::chrono::high_resolution_clock::now();
  worker_node_->Send(NodeRole::SERVER, 0, message);
  auto end = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "send ok, cost:" << (end - start).count() / 1e6 << "ms";

  CommMessage comm_message;
  auto start2 = std::chrono::high_resolution_clock::now();
  worker_node_->Send(NodeRole::SERVER, 0, message, &comm_message);
  std::cout << comm_message.pb_meta().role() << std::endl;
  std::cout << comm_message.pb_meta().rank_id() << std::endl;
  auto end2 = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "send ok, cost:" << (end2 - start2).count() / 1e6 << "ms";
  worker_node_->Finish();
  worker_node_->Stop();
}

}  // namespace core
}  // namespace ps

}  // namespace mindspore