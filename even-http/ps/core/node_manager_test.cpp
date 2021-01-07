//
// Created by cds on 2020/10/24.
//

#include "node_manager_test.h"
#include "ps/core/thread_pool.h"

namespace mindspore {
namespace ps {
namespace core {
void NodeManagerTest::StartScheduler() {
  scheduler_node_ = std::make_unique<SchedulerNode>();
  scheduler_node_->Start();
  scheduler_node_->Finish();
  scheduler_node_->Stop();
}

void NodeManagerTest::StartServer() {
  int result = evthread_use_pthreads();
  if (result != 0) {
    MS_LOG(EXCEPTION) << "Use event pthread failed!";
  }
  server_node_ = std::make_unique<ServerNode>();
  server_node_->set_event_callback([&](const NodeEvent &event) {
    if (event == NodeEvent::CLUSTER_TIMEOUT) {
      MS_LOG(INFO) << "111111111111111111111111111111!";
      //      server_node_->Finish();
      //      server_node_->Stop();
    }
  });
  server_node_->set_handler([&](std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> message) {
    KVMessage kv_message;
    kv_message.ParseFromString(message->data());
    MS_LOG(INFO) << "size:" << kv_message.keys_size();

    server_node_->Response(conn, message);
  });
  server_node_->Start();
  //  std::this_thread::sleep_for(std::chrono::seconds(10));
  //  KVMessage kv_message;
  //  std::vector<int> keys(66, 1);
  //  std::vector<int> values(66, 2);
  //  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  //  *kv_message.mutable_values() = {values.begin(), values.end()};
  //  auto start = std::chrono::high_resolution_clock::now();
  //  server_node_->Broadcast(NodeRole::SERVER, kv_message.SerializeAsString());
  //  auto end = std::chrono::high_resolution_clock::now();
  //  MS_LOG(INFO) << "server node broadcast send ok, cost:" << (end - start).count() / 1e6 << "ms";

  // KVMessage kv_message;
  // std::vector<int> keys(33, 1);
  // std::vector<int> values(33, 2);
  // *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  // *kv_message.mutable_values() = {values.begin(), values.end()};
  // auto start = std::chrono::high_resolution_clock::now();
  // uint64_t request = server_node_->CollectiveSendAsync(NodeRole::SERVER, 1, kv_message.SerializeAsString());
  // std::string receive_data;
  // server_node_->CollectiveWait(server_node_->CollectiveReceiveAsync(NodeRole::SERVER, 1, &receive_data));
  // server_node_->Wait(request);
  // KVMessage kvMessage;
  // kvMessage.ParseFromString(receive_data);
  // std::cout << kvMessage.keys_size() << std::endl;
  // auto end = std::chrono::high_resolution_clock::now();
  // MS_LOG(INFO) << "server node CollectiveSend ok, cost:" << (end - start).count() / 1e6 << "ms";

  std::this_thread::sleep_for(std::chrono::seconds(600000));
  server_node_->Finish(300);
  server_node_->Stop();
}

void NodeManagerTest::StartServer1() {
  int result = evthread_use_pthreads();
  if (result != 0) {
    MS_LOG(EXCEPTION) << "Use event pthread failed!";
  }
  server_node_ = std::make_unique<ServerNode>();
  server_node_->set_event_callback([&](const NodeEvent &event) {
    if (event == NodeEvent::CLUSTER_TIMEOUT) {
      MS_LOG(INFO) << "111111111111111111111111111111!";
      //      server_node_->Finish();
      //      server_node_->Stop();
    }
  });

  ThreadPool pool(2);

  server_node_->set_handler([&](std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> message) {
    KVMessage kv_message;
    kv_message.ParseFromString(message->data());
    MS_LOG(INFO) << "size:" << kv_message.keys_size();
    pool.Submit(&NodeManagerTest::ThreadResponse, this, conn, message);
    // server_node_->Response(conn, message);
  });
  server_node_->Start();
  //  std::this_thread::sleep_for(std::chrono::seconds(10));
  // KVMessage kv_message;
  // std::vector<int> keys(55, 1);
  // std::vector<int> values(55, 2);
  // *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  // *kv_message.mutable_values() = {values.begin(), values.end()};
  // auto start = std::chrono::high_resolution_clock::now();
  // uint64_t request = server_node_->CollectiveSendAsync(NodeRole::SERVER, 0, kv_message.SerializeAsString());
  // std::string receive_data;
  // server_node_->CollectiveWait(server_node_->CollectiveReceiveAsync(NodeRole::SERVER, 0, &receive_data));
  // server_node_->Wait(request);
  // KVMessage kvMessage;
  // kvMessage.ParseFromString(receive_data);
  // std::cout << kvMessage.keys_size() << std::endl;
  // auto end = std::chrono::high_resolution_clock::now();
  // MS_LOG(INFO) << "server node collective send ok, cost:" << (end - start).count() / 1e6 << "ms";
  std::this_thread::sleep_for(std::chrono::seconds(600000));
  server_node_->Finish(300);
  server_node_->Stop();
}

void NodeManagerTest::ThreadResponse(std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> message) {
  MS_LOG(INFO) << "thred id:" << std::this_thread::get_id();
  server_node_->Response(conn, message);
}

void NodeManagerTest::StartClient() {
  int result = evthread_use_pthreads();
  if (result != 0) {
    MS_LOG(EXCEPTION) << "Use event pthread failed!";
  }
  worker_node_ = std::make_unique<WorkerNode>();
  worker_node_->set_event_callback([&](const NodeEvent &event) {
    if (event == NodeEvent::CLUSTER_TIMEOUT) {
      MS_LOG(INFO) << "NODE_TIMEOUT, finish!";
      std::this_thread::sleep_for(std::chrono::milliseconds(50000));
      //      worker_node_->Finish();
      //      worker_node_->Stop();
    }
  });
  worker_node_->Start();
  KVMessage kv_message;
  std::vector<int> keys(1000000, 1);
  std::vector<int> values(1000000, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};

  //  std::this_thread::sleep_for(std::chrono::seconds(10));

  auto start1 = std::chrono::high_resolution_clock::now();
  const auto &message = kv_message.SerializeAsString();
  auto end1 = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "serialize, cost:" << (end1 - start1).count() / 1e6 << "ms";

  //  auto start = std::chrono::high_resolution_clock::now();
  //  worker_node_->Send(NodeRole::SERVER, 0, message);
  //  auto end = std::chrono::high_resolution_clock::now();
  //  MS_LOG(INFO) << "send ok, cost:" << (end - start).count() / 1e6 << "ms";

  std::string comm_message;
  KVMessage resp_temp;
  auto start2 = std::chrono::high_resolution_clock::now();

  worker_node_->Send(NodeRole::SERVER, 0, message, &comm_message);

  //  std::cout << comm_message.pb_meta().role() << std::endl;
  //  std::cout << comm_message.pb_meta().rank_id() << std::endl;
  resp_temp.ParseFromString(comm_message);
  std::cout << resp_temp.keys_size() << std::endl;
  auto end2 = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "send ok, cost:" << (end2 - start2).count() / 1e6 << "ms";

  //  auto start = std::chrono::high_resolution_clock::now();
  //  worker_node_->BroadcastToServers(kv_message.SerializeAsString());
  //  auto end = std::chrono::high_resolution_clock::now();
  //  MS_LOG(INFO) << "send ok, cost:" << (end - start).count() / 1e6 << "ms";

  for (int i = 0; i < 2000000; i++) {
    std::vector<uint32_t> rank_ids = {0, 1};
    KVMessage kv_message1;
    std::vector<int> keys1(100, 1);
    std::vector<int> values1(100, 2);
    *kv_message1.mutable_keys() = {keys1.begin(), keys1.end()};
    *kv_message1.mutable_values() = {values1.begin(), values1.end()};
    std::vector<std::string> data = {kv_message1.SerializeAsString(), kv_message.SerializeAsString()};
    std::vector<std::string> resp;
    worker_node_->Send(NodeRole::SERVER, rank_ids, data, &resp);
    KVMessage kv_message_resp;
    KVMessage kv_message_resp1;
    kv_message_resp.ParseFromString(resp.at(0));
    kv_message_resp1.ParseFromString(resp.at(1));
    MS_LOG(INFO) << "index:" << i << ",resp size:" << kv_message_resp.keys_size()
                 << " resp1 size:" << kv_message_resp1.keys_size();
  }
  worker_node_->Finish();
  worker_node_->Stop();
}

}  // namespace core
}  // namespace ps

}  // namespace mindspore