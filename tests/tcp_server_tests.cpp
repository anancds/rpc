//
// Created by cds on 2020/10/19.
//

#include <tcp_client.h>
#include <tcp_server.h>
#include <thread>
#include "common_test.h"

namespace mindspore {
namespace ps {
namespace comm {
class TestTcpServer : public UT::Common {
 public:
  TestTcpServer() = default;
  void SetUp() override {
    server_ = new TcpMessageServer("127.0.0.1", 9000);
    std::unique_ptr<std::thread> http_server_thread_(nullptr);
    http_server_thread_ = std::make_unique<std::thread>([&]() {
      server_->ReceiveMessage([](const TcpMessageServer &server, const TcpMessageConnection &conn,
                                 const void *buffer, size_t num) {
        EXPECT_STREQ(std::string(reinterpret_cast<const char *>(buffer), num).c_str(), "TCP_MESSAGE");
        TcpMessageServer::SendMessage(conn, buffer, num);
      });
      server_->InitServer();
      server_->Start();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    http_server_thread_->detach();
  }
  void TearDown() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    client_->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    server_->Stop();
  }

  TcpMessageClient *client_;
  TcpMessageServer *server_;
  const std::string test_message_ = "TCP_MESSAGE";
};

TEST_F(TestTcpServer, ServerSendeMessage) {
  client_ = new TcpMessageClient("127.0.0.1", 9000);
  std::unique_ptr<std::thread> http_client_thread(nullptr);
  http_client_thread = std::make_unique<std::thread>([&]() {
    client_->ReceiveMessage([](const TcpMessageClient &client, const void *buffer, size_t num) {
      EXPECT_STREQ(std::string(reinterpret_cast<const char *>(buffer), num).c_str(), "TCP_MESSAGE");
    });

    client_->InitTcpClient();
    client_->SendMessage(test_message_.c_str(), test_message_.size());
    client_->Start();
  });
  http_client_thread->detach();
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore