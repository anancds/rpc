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

#ifndef MINDSPORE_CCSRC_PS_THREAD_POOL_THREAD_POOL_H_
#define MINDSPORE_CCSRC_PS_THREAD_POOL_THREAD_POOL_H_

#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace mindspore {
namespace ps {
namespace core {

class ThreadPool {
 public:
  ThreadPool(size_t thread_num, size_t max_task_num = 10);
  ~ThreadPool();

  template <typename F, typename... Args>
  void Submit(F &&f, Args &&...args) {
    auto callee = std::bind(f, args...);
    std::function<void()> task = [callee]() -> void { callee(); };
    {
      while (true) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (task_num_ >= max_task_num_) {
          lock.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(4));
        } else {
          break;
        }
      }
      std::unique_lock<std::mutex> lock(mtx_);
      task_num_++;
      task_queue_.push(task);
    }
  }

 private:
  bool running_;
  size_t thread_num_;
  size_t max_task_num_;

  size_t task_num_;
  size_t idle_thread_num_;
  std::thread notify_thread_;

  std::vector<std::thread> working_threads_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::queue<std::function<void()>> task_queue_;
};
}  // namespace core
}  // namespace ps
}  // namespace mindspore
#endif  // MINDSPORE_CCSRC_PS_THREAD_POOL_THREAD_POOL_H_