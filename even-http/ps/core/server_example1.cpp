
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "ps/core/node_manager_test.h"

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"

using namespace mindspore::ps::core;
using namespace mindspore::ps;

static void Start() {
  ClusterConfig::Init(1, 2, std::make_unique<std::string>("127.0.0.1"), 9999);
  NodeManagerTest::Get()->StartServer();
}

int main(int /*argc*/, char ** /*argv*/) {
  Start();

  return EXIT_SUCCESS;
}
