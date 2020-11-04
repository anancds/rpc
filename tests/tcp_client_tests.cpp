#include <memory>
#include "comm/tcp_client.h"
#include "common_test.h"

namespace mindspore {
namespace ps {
namespace comm {
class TestTcpClient : public UT::Common {
 public:
  TestTcpClient() = default;
  void SetUp() override { std::cout << std::endl; }
  void TearDown() override { std::cout << std::endl; }
};

TEST_F(TestTcpClient, InitClientPortErrorNoException) {
  auto client = std::make_unique<TcpMessageClient *>(new TcpMessageClient("127.0.0.1", -1));
  (*client)->ReceiveMessage([](const mindspore::ps::comm::TcpMessageClient &client, const void *buffer, size_t num) {
    client.SendMessage(buffer, num);
  });

  EXPECT_NO_THROW((*client)->InitTcpClient());
}

TEST_F(TestTcpClient, InitClientIPError) {
  auto client = std::make_unique<TcpMessageClient *>(new TcpMessageClient("127.0.0.13543", 9000));
  (*client)->ReceiveMessage([](const mindspore::ps::comm::TcpMessageClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
    client.SendMessage(buffer, num);
  });

  ASSERT_THROW((*client)->InitTcpClient(), std::exception);
}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore