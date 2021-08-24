
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "ps/core/node_manager_test.h"
#include "utils/ms_utils.h"

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"

using namespace mindspore::ps::core;
using namespace mindspore::ps;

const std::string test_message(1024, 's');
using Key = uint64_t;
static void Start() {
  ClusterConfig::Init(1, 1, "127.0.0.1", 9999);
  NodeManagerTest::Get()->StartClient();
}

int main(int /*argc*/, char ** /*argv*/) {
  std::strtol(mindspore::common::GetEnv("").c_str(), nullptr, 10);
  Start();

  return EXIT_SUCCESS;
}
