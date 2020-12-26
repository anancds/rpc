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

#include "common/common_test.h"
#define protected public
#include "ps/core/worker_node.h"
#undef protected

namespace mindspore {
namespace ps {
namespace core {
class TestAbstractNode : public UT::Common {
 public:
  TestAbstractNode() = default;
  virtual ~TestAbstractNode() = default;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TestAbstractNode, NextExpectedRankRequestId) {
  WorkerNode workerNode;
  ASSERT_EQ(1, workerNode.NextExpectedRankRequestId(0));
  ASSERT_EQ(2, workerNode.NextExpectedRankRequestId(0));
  ASSERT_EQ(1, workerNode.NextExpectedRankRequestId(1));
  uint64_t a = workerNode.NextExpectedRankRequestId(0);
  workerNode.set_received_data_callback(0, a, [&](){
    std::cout << a << std::endl;
  });
  workerNode.received_callbacks_[std::make_pair(0, a)]();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore