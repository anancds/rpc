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

#ifndef RPC_COUNT_DOWN_LATCH_H
#define RPC_COUNT_DOWN_LATCH_H

#include <mutex>
#include <condition_variable>

namespace mindspore {
namespace ps {
namespace core {
class CountDownLatch {
 public:
  explicit CountDownLatch(int count) : count_(count) {}

  void Wait();

  bool WaitFor(const uint32_t &timeout);

  void CountDown();

  int GetCount() const;

 private:
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  int count_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore
#endif  // RPC_COUNT_DOWN_LATCH_H
