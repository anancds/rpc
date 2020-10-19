//
// Created by cds on 2020/10/19.
//

#include "common_test.h"
namespace mindspore {
namespace ps {
namespace comm {
class TestTcpServer : public UT::Common {
 public:
  TestTcpServer() = default;
  void SetUp() override { std::cout << std::endl; }
  void TearDown() override { std::cout << std::endl; }
};

TEST_F(TestTcpServer, init) {}
}  // namespace comm
}  // namespace ps
}  // namespace mindspore