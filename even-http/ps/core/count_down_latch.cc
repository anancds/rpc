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

#include "ps/core/count_down_latch.h"

namespace mindspore {
namespace ps {
namespace core {

void CountDownLatch::wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  if (count_ > 0) {
    condition_.wait(lock, [&] { return count_ == 0; });
  }
}

void CountDownLatch::countDown() {
  std::lock_guard<std::mutex> lock(mutex_);
  --count_;
  if (count_ == 0) {
    condition_.notify_all();
  }
}

int CountDownLatch::getCount() {
  std::lock_guard<std::mutex> lock(mutex_);
  return count_;
}
}  // namespace core
}  // namespace ps
}  // namespace mindspore
