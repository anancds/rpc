#include <memory>
#include "ps/comm/tcp_client.h"
#include "common_test.h"

namespace mindspore {
namespace ps {
namespace comm {
class TestTcpClient : public UT::Common {
 public:
  TestTcpClient() = default;
};

TEST_F(TestTcpClient, InitClientIPError) {
  auto client = std::make_unique<TcpClient>("127.0.0.13543", 9000);

  client->SetMessageCallback([](const TcpClient &client, const CommMessage &message) { client.SendMessage(message); });

  ASSERT_THROW(client->Init(), std::exception);
}

TEST_F(TestTcpClient, InitClientPortErrorNoException) {
  auto client = std::make_unique<TcpClient>("127.0.0.1", -1);

  client->SetMessageCallback([](const TcpClient &client, const CommMessage &message) { client.SendMessage(message); });

  EXPECT_NO_THROW(client->Init());
}

}  // namespace comm
}  // namespace ps
}  // namespace mindspore