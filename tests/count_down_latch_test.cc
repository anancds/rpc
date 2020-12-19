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
#include <thread>

#include "common/common_test.h"
#include "ps/core/count_down_latch.h"

namespace mindspore {
namespace ps {
namespace core {
class TestCountDownLatch : public UT::Common {
 public:
  TestCountDownLatch() = default;
  virtual ~TestCountDownLatch() = default;

  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TestCountDownLatch, CountDownLatch) {
  CountDownLatch count_down_latch(2);
  ASSERT_EQ(count_down_latch.GetCount(), 2);
  std::thread count_down_thread([&](){
    std::this_thread::sleep_for(std::chrono::seconds(2));
    count_down_latch.CountDown();
    count_down_latch.CountDown();
  });
  count_down_latch.Wait();
  if (count_down_thread.joinable()) {
    count_down_thread.join();
  }
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore