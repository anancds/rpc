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

#include "comm/tcp_client.h"
#include "comm/tcp_server.h"
#include "common_test.h"

#include <memory>
#include <thread>

namespace mindspore {
namespace ps {
namespace comm {
class TestTcpKVServer : public UT::Common {
 public:
  TestTcpKVServer() : client_(nullptr), server_(nullptr) {}
  virtual ~TestTcpKVServer() = default;

  void SetUp() override {
    server_ = std::make_unique<TcpKVServer *>(new TcpKVServer("127.0.0.1", 9998));
    std::unique_ptr<std::thread> http_server_thread_(nullptr);
    http_server_thread_ = std::make_unique<std::thread>([&]() {
      (*server_)->ReceiveKVMessage([](const TcpKVServer &server, const TcpKVConnection &conn, const PBMessage &message) {
        MS_LOG(INFO) << "The server message size is:" << message.pb_kv_message().keys_size();
        server.SendKVMessage(conn, message);
      });
      (*server_)->InitServer();
      (*server_)->Start();
    });
    http_server_thread_->detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  }
  void TearDown() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    (*client_)->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    (*server_)->Stop();
  }

  std::unique_ptr<TcpKVClient*> client_;
  std::unique_ptr<TcpKVServer*> server_;
};

TEST_F(TestTcpKVServer, KVServerSendMessage) {
  client_ = std::make_unique<TcpKVClient*>(new TcpKVClient("127.0.0.1", 9998));
  std::unique_ptr<std::thread> http_client_thread(nullptr);
  http_client_thread = std::make_unique<std::thread>([&]() {
    (*client_)->ReceiveKVMessage(
      [](const TcpKVClient &client, const CommMessage &message) {
        KVMessage kv_message;
        kv_message.ParseFromString(message.data());
        EXPECT_EQ(2, kv_message.keys_size());
      });

    (*client_)->InitTcpClient();
    CommMessage comm_message;
    KVMessage kv_message;
    std::vector<int> keys{1, 2};
    std::vector<int> values{3, 4};
    *kv_message.mutable_keys() = {keys.begin(), keys.end()};
    *kv_message.mutable_values() = {values.begin(), values.end()};

    comm_message.set_data(kv_message.SerializeAsString());
    //  int num = 10;
    //  uint64_t keys[num];
    //  float vals[num];
    //
    //  for (int i = 0; i < num; ++i) {
    //    keys[i] = (rand() % 1000);
    //    vals[i] = (rand() % 1000);
    //  }
    //  message.SetArrayData(keys, vals, 10, 10);
    (*client_)->SendKVMessage(comm_message);
    (*client_)->Start();
  });
  http_client_thread->detach();
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore