
#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"
#include "../../../build/even-http/ps/core/test.pb.h"
#include "ps/core/scheduler_node.h"
#include "ps/core/file_configuration.h"


#include "nlohmann/json.hpp"
#include <unordered_map>

using namespace mindspore::ps::core;
using namespace mindspore::ps;
using VectorPtr = std::shared_ptr<std::vector<unsigned char>>;

void TestFileConfiguration() {

    FileConfiguration file_config("/home/cds/test.json");
    file_config.Put("a", "b");
    std::cout << file_config.Get("aa", "aaa");
}

int main(int argc, char **argv) {

    TestFileConfiguration();

}