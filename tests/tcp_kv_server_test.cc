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

#include "tcp_client.h"
#include "tcp_server.h"
#include "common_test.h"

#include <thread>

namespace mindspore {
namespace ps {
namespace comm {
class TestTcpKVServer : public UT::Common {
 public:
  TestTcpKVServer() = default;
  void SetUp() override {
    server_ = new TcpKVServer("127.0.0.1", 9998);
    std::unique_ptr<std::thread> http_server_thread_(nullptr);
    http_server_thread_ = std::make_unique<std::thread>([&]() {
      server_->ReceiveKVMessage([](const TcpKVServer &server, const TcpKVConnection &conn, const Message &message) {
        server.SendKVMessage(conn, message);
      });
      server_->InitServer();
      server_->Start();
    });
    http_server_thread_->detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  }
  void TearDown() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    client_->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    server_->Stop();
  }

  TcpKVClient *client_;
  TcpKVServer *server_;
};

TEST_F(TestTcpKVServer, KVServerSendMessage) {
client_ = new TcpKVClient("127.0.0.1", 9998);
std::unique_ptr<std::thread> http_client_thread(nullptr);
http_client_thread = std::make_unique<std::thread>([&]() {
  client_->ReceiveKVMessage([](const TcpKVClient &client, const Message &message) {
    MS_LOG(INFO) << "The message size is:" << message.key_len_;
  });

  client_->InitTcpClient();
  Message message{};
  int num = 10;
  uint64_t keys[num];
  float vals[num];

  for (int i = 0; i < num; ++i) {
    keys[i] = (rand() % 1000);
    vals[i] = (rand() % 1000);
  }
  message.AddArrayData(keys, vals, 10, 10);
  client_->SendKVMessage(message);
  client_->Start();
});
http_client_thread->detach();
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore