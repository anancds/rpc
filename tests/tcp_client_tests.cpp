

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

TEST_F(TestTcpClient, InitClientPortError) {
  TcpClient *client = new TcpClient("127.0.0.1", -1);
  client->ReceiveMessage([](mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
    client.SendMessage(buffer, num);
  });

  ASSERT_THROW(client->InitTcpClient(), std::exception);
}

TEST_F(TestTcpClient, InitClientIPError) {
  TcpClient *client = new TcpClient("127.0.0.13543", 9000);
  client->ReceiveMessage([](mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
    client.SendMessage(buffer, num);
  });

  ASSERT_THROW(client->InitTcpClient(), std::exception);
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore