
#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "../../../build/even-http/ps/core/test.pb.h"
#include "ps/core/scheduler_node.h"
#include "ps/core/file_configuration.h"

#include "nlohmann/json.hpp"
#include <unordered_map>
#include <deque>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace mindspore::ps::core;
using namespace mindspore::ps;
using VectorPtr = std::shared_ptr<std::vector<unsigned char>>;

enum class StorageType : int { kFileStorage = 1, kDatabase = 2 };

std::mutex mtx;
std::condition_variable cv;

void LockThread() {
  std::cout << "before" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));
  mtx.lock();
  std::cout << "after" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));
  mtx.unlock();
}

void TestWait() {
  std::thread th(LockThread);
  std::unique_lock<std::mutex> lock(mtx);
  cv.wait_for(lock, std::chrono::seconds(4), [] {
    return false;
  });
  std::cout << "aaaa" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(1199));
}

void TestFileConfiguration() {
  FileConfiguration file_config("/home/cds/test.json");
  file_config.Initialize();
  file_config.Put("aa", "bb");
  std::cout << file_config.Get("recovery", "aaa") << std::endl;
  std::cout << file_config.Get("aa", "aaa") << std::endl;
}

void TestDeque() {
  std::deque<int> queue;
  queue.push_back(1);
  queue.push_back(2);
  std::cout << queue.front() << std::endl;
  queue.pop_front();
  std::cout << queue.size() << std::endl;
}

void TestFindIf() {
  std::vector<int> temp = {1, 2, 3, 4, 5};
  int rank_id = -1;
  auto rank_it = std::find_if(temp.begin(), temp.end(), [&rank_id](auto item) {
    if (item > 2) {
      rank_id = item;
      return true;

    } else {
      return false;
    }
  });

  if (rank_it != temp.end()) {
    std::cout << rank_id << std::endl;
  }

  auto num = std::count_if(temp.begin(), temp.end(), [](auto item) { return item > 1; });
  std::cout << num << std::endl;
}

void TestIp() {
  std::cout << CommUtil::CheckIp("255.255.255.255") << std::endl;
  std::cout << CommUtil::CheckIp("0.0.0.0") << std::endl;
  std::cout << CommUtil::CheckIp("127.0.0.1") << std::endl;
}

int main(int argc, char **argv) {
  // TestFileConfiguration();
  // TestDeque();
  // TestFindIf();

  // TestIp();
  TestWait();
}