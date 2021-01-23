//
// Created by cds on 2020/10/24.
//

#include "node_manager_test.h"
#include "ps/core/thread_pool.h"

namespace mindspore {
namespace ps {
namespace core {
void NodeManagerTest::StartScheduler() {
  scheduler_node_ = std::make_shared<SchedulerNode>();
  scheduler_node_->Start();
  scheduler_node_->Finish();
  scheduler_node_->Stop();
}

void NodeManagerTest::CollectiveTest(const uint32_t &rank_id) {
  KVMessage kv_message;
  std::vector<int> keys(33, 1);
  std::vector<int> values(33, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};

  const void *temp = kv_message.SerializeAsString().data();
  size_t size = kv_message.ByteSizeLong();

  auto start = std::chrono::high_resolution_clock::now();
  uint64_t request = server_node_->CollectiveSendAsync(NodeRole::SERVER, rank_id, temp, size);
  void *receive_data;
  size_t len;
  server_node_->CollectiveWait(server_node_->CollectiveReceiveAsync(NodeRole::SERVER, rank_id, &receive_data, &len),
                               10);
  server_node_->Wait(request, 10);
  KVMessage kvMessage;
  kvMessage.ParseFromArray(receive_data, len);
  MS_LOG(INFO) << kvMessage.keys_size();
  auto end = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "server node CollectiveSend ok, cost:" << (end - start).count() / 1e6 << "ms";
}

void NodeManagerTest::BroadCastTest() {
  KVMessage kv_message;
  std::vector<int> keys(66, 1);
  std::vector<int> values(66, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  CommMessage message;
  message.set_data(kv_message.SerializeAsString());
  auto start = std::chrono::high_resolution_clock::now();
  // server_node_->Broadcast(NodeRole::SERVER, message);
  auto end = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "server node broadcast send ok, cost:" << (end - start).count() / 1e6 << "ms";
}

size_t NodeManagerTest::PushTest(const uint32_t &size) {
  KVMessage kv_message;
  std::vector<int> keys(size, 1);
  std::vector<int> values(size, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};

  std::string data = kv_message.SerializeAsString();

  std::shared_ptr<unsigned char> res(new unsigned char[data.length()]);
  memcpy_s(res.get(), data.length(), data.data(), data.length());
  // auto end1 = std::chrono::high_resolution_clock::now();
  // MS_LOG(INFO) << "serialize, cost:" << (end1 - start1).count() / 1e6 << "ms";

  auto start = std::chrono::high_resolution_clock::now();
  worker_node_->Send(NodeRole::SERVER, 0, res, data.length(), 0);
  auto end = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "PushTest, cost:" << (end - start).count() / 1e6 << "ms";
  return (end - start).count() / 1e6;
}

void NodeManagerTest::PullTest(const uint32_t &size) {
  KVMessage kv_message;
  std::vector<int> keys(size, 1);
  std::vector<int> values(size, 2);
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  CommMessage comm_message;
  comm_message.set_data(kv_message.SerializeAsString());
  CommMessage comm_resp_message;
  auto start2 = std::chrono::high_resolution_clock::now();

  std::shared_ptr<std::vector<unsigned char>> output = std::make_shared<std::vector<unsigned char>>();

  // worker_node_->Send(NodeRole::SERVER, 0, comm_message, output);
  KVMessage kv_resp_message;
  kv_resp_message.ParseFromArray(output->data(), output->size());
  std::cout << kv_resp_message.keys_size() << std::endl;
  auto end2 = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "send ok, cost:" << (end2 - start2).count() / 1e6 << "ms";
}

void NodeManagerTest::MultiPullTest(const uint32_t &size) {
  // for (int i = 0; i < 2; i++) {
  //   std::vector<uint32_t> rank_ids = {0, 1};
  //   KVMessage kv_message1;
  //   std::vector<int> keys1(100, 1);
  //   std::vector<int> values1(100, 2);
  //   *kv_message1.mutable_keys() = {keys1.begin(), keys1.end()};
  //   *kv_message1.mutable_values() = {values1.begin(), values1.end()};
  //   std::vector<std::string> data = {kv_message1.SerializeAsString(), kv_message.SerializeAsString()};
  //   std::vector<std::string> resp;
  //   worker_node_->Send(NodeRole::SERVER, rank_ids, data, &resp);
  //   KVMessage kv_message_resp;
  //   KVMessage kv_message_resp1;
  //   kv_message_resp.ParseFromString(resp.at(0));
  //   kv_message_resp1.ParseFromString(resp.at(1));
  //   MS_LOG(INFO) << "index:" << i << ",resp size:" << kv_message_resp.keys_size()
  //                << " resp1 size:" << kv_message_resp1.keys_size();
  // }
}

void NodeManagerTest::PackKVMessage(std::shared_ptr<CommMessage> message) {
  auto start1 = std::chrono::high_resolution_clock::now();
  KVMessage kv_message1;
  std::vector<int> keys(262144, 1);
  *kv_message1.mutable_keys() = {keys.begin(), keys.end()};
  auto start2 = std::chrono::high_resolution_clock::now();
  message->set_data(kv_message1.SerializeAsString());
  auto end1 = std::chrono::high_resolution_clock::now();
  MS_LOG(INFO) << "serialize, cost:" << (end1 - start1).count() / 1e6 << "ms";
  MS_LOG(INFO) << "serialize, cost:" << (end1 - start2).count() / 1e6 << "ms";
}

void NodeManagerTest::PackMessage(std::shared_ptr<std::vector<unsigned char>> message) {
  // std::shared_ptr<std::vector<unsigned char>>temp()
}

void NodeManagerTest::StartServer() {
  int result = evthread_use_pthreads();
  if (result != 0) {
    MS_LOG(EXCEPTION) << "Use event pthread failed!";
  }
  server_node_ = std::make_shared<ServerNode>();
  server_node_->set_event_callback([&](const NodeEvent &event) {
    if (event == NodeEvent::CLUSTER_TIMEOUT) {
      MS_LOG(INFO) << "111111111111111111111111111111!";
      //      server_node_->Finish();
      //      server_node_->Stop();
    }
  });
  server_node_->set_handler(
    [&](std::shared_ptr<TcpConnection> conn, std::shared_ptr<MessageMeta> meta, DataPtr data, size_t size) {
      KVMessage kv_message;
      // kv_message.ParseFromString(message->data());
      // MS_LOG(INFO) << "size:" << kv_message.keys_size();
      // PackKVMessage(message);

      // auto res = std::make_shared<std::vector<unsigned char>>(262144, 1);
      std::shared_ptr<unsigned char> res(new unsigned char[0]);

      server_node_->Response(conn, meta, res, 0);
    });
  server_node_->Start();

  // BroadCastTest();

  ThreadPool pool_coll1(10);
  for (int i = 0; i < 100; ++i) {
    // pool_coll1.Submit(&NodeManagerTest::CollSend, this, 1);
    CollectiveTest(1);
  }

  // std::this_thread::sleep_for(std::chrono::seconds(600000));
  server_node_->Finish(30);
  server_node_->Stop();
}

void NodeManagerTest::StartServer1() {
  server_node_ = std::make_shared<ServerNode>();
  server_node_->set_event_callback([&](const NodeEvent &event) {
    if (event == NodeEvent::CLUSTER_TIMEOUT) {
      MS_LOG(INFO) << "111111111111111111111111111111!";
      //      server_node_->Finish();
      //      server_node_->Stop();
    }
  });

  ThreadPool pool(10);

  server_node_->set_handler(
    [&](std::shared_ptr<TcpConnection> conn, std::shared_ptr<MessageMeta> meta, DataPtr data, size_t size) {
      KVMessage kv_message;
      // kv_message.ParseFromArray(data->data(), data->size());
      MS_LOG(INFO) << "size:" << kv_message.keys_size();
      // auto res = std::make_shared<std::vector<unsigned char>>(262144, 1);
      // auto data1 = std::make_shared<std::vector<unsigned char>>();
      pool.Submit(&NodeManagerTest::ThreadResponse, this, conn, meta, data, size);
    });
  server_node_->Start();

  ThreadPool pool_coll(10);
  for (int i = 0; i < 100; ++i) {
    // pool_coll.Submit(&NodeManagerTest::CollSend, this, 0);
    CollectiveTest(0);
  }

  server_node_->Finish(30);
  server_node_->Stop();
}

void NodeManagerTest::ThreadResponse(std::shared_ptr<TcpConnection> conn, std::shared_ptr<MessageMeta> meta,
                                     DataPtr data, size_t size) {
  MS_LOG(INFO) << "thred id:" << std::this_thread::get_id();
  server_node_->Response(conn, meta, data, size);
}

void NodeManagerTest::CollSend(const u_int32_t &rank_id) { CollectiveTest(rank_id); }

void NodeManagerTest::StartClient() {
  int result = evthread_use_pthreads();
  if (result != 0) {
    MS_LOG(EXCEPTION) << "Use event pthread failed!";
  }
  worker_node_ = std::make_shared<WorkerNode>();
  worker_node_->set_event_callback([&](const NodeEvent &event) {
    if (event == NodeEvent::CLUSTER_TIMEOUT) {
      MS_LOG(INFO) << "NODE_TIMEOUT, finish!";
      std::this_thread::sleep_for(std::chrono::milliseconds(50000));
      //      worker_node_->Finish();
      //      worker_node_->Stop();
    }
  });
  worker_node_->Start();

  size_t time = 0;
  for (int i = 0; i < 1; i++) {
    time += PushTest(262144);
  }
  MS_LOG(INFO) << "Push total cost:" << time;

  worker_node_->Finish(30);
  worker_node_->Stop();
}

}  // namespace core
}  // namespace ps

}  // namespace mindspore