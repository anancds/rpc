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

#include "common_test.h"
#include "ps/core/comm_util.h"

#include <memory>
#include <thread>

namespace mindspore {
namespace ps {
namespace core {
class TestCommUtil : public UT::Common {
 public:
  TestCommUtil() = default;
  virtual ~TestCommUtil() = default;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TestCommUtil, GetAvailableInterfaceAndIP) {
  std::string interface;
  std::string ip;
  CommUtil::GetAvailableInterfaceAndIP(&interface, &ip);
  EXPECT_TRUE(!interface.empty());
  EXPECT_TRUE(!ip.empty());
}

TEST_F(TestCommUtil, ValidateRankId) {
  ClusterConfig::Init(3, 2, std::make_unique<std::string>("127.0.0.1"), 9999);
  EXPECT_TRUE(CommUtil::ValidateRankId(NodeRole::WORKER, 2));
  EXPECT_FALSE(CommUtil::ValidateRankId(NodeRole::WORKER, 3));
  EXPECT_TRUE(CommUtil::ValidateRankId(NodeRole::SERVER, 1));
  EXPECT_FALSE(CommUtil::ValidateRankId(NodeRole::SERVER, 2));
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore