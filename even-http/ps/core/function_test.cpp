
#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "../../../build/even-http/ps/core/test.pb.h"
#include "ps/core/scheduler_node.h"
#include "ps/core/file_configuration.h"

#include "nlohmann/json.hpp"
#include <unordered_map>
#include <deque>

using namespace mindspore::ps::core;
using namespace mindspore::ps;
using VectorPtr = std::shared_ptr<std::vector<unsigned char>>;

enum class StorageType : int { kFileStorage = 1, kDatabase = 2 };

void TestFileConfiguration() {
  FileConfiguration file_config("/home/cds/test.json");
  file_config.Put("a", "b");
  std::cout << file_config.Get("aa", "aaa");
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

int main(int argc, char **argv) {
  // TestFileConfiguration();
  // TestDeque();
  TestFindIf();

  std::string b = "aflsdj";
  auto a = std::strtol(b.c_str(), nullptr, 10);
  std::cout << a << std::endl;
}