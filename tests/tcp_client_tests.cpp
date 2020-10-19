//
// Created by cds on 2020/10/19.
//

//
// Created by cds on 2020/10/19.
//

#include "common_test.h"
#include "tcp_client.h"

namespace mindspore {
namespace ps {
namespace comm {
class TestTcpClient : public UT::Common {
 public:
  TestTcpClient() = default;
  void SetUp() override { std::cout << std::endl; }
  void TearDown() override { std::cout << std::endl; }
};

TEST_F(TestTcpClient, InitClientColonMiss) {
  mindspore::ps::comm::TcpClient client;
  client.SetMessageCallback([](mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
    client.SendMessage(buffer, num);
  });

  client.SetTarget("127.0.0.19000");
  ASSERT_THROW(client.InitTcpClient(), std::exception);
}

TEST_F(TestTcpClient, InitClientPortError) {
  mindspore::ps::comm::TcpClient client;
  client.SetMessageCallback([](mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
    client.SendMessage(buffer, num);
  });

  client.SetTarget("127.0.0.1:-1");
  ASSERT_THROW(client.InitTcpClient(), std::exception);
}

TEST_F(TestTcpClient, InitClientIPError) {
  mindspore::ps::comm::TcpClient client;
  client.SetMessageCallback([](mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
    client.SendMessage(buffer, num);
  });

  client.SetTarget("127.0.0.1423432:9000");
  ASSERT_THROW(client.InitTcpClient(), std::exception);
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore