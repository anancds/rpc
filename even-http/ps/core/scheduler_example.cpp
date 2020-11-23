//
// Created by cds on 2020/10/16.
//

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "../../../build/even-http/ps/core/comm.pb.h"
#include "message.h"
#include "ps/core/node_manager.h"
#include "tcp_server.h"

using namespace mindspore::ps::core;

static void StartServer() {
  ClusterConfig::Init(1, 1, std::make_unique<std::string>("127.0.0.1"), 9999);
  NodeManager::Get()->StartScheduler();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}


int main(int /*argc*/, char ** /*argv*/) {
  std::string test = "127.0.0.1:123";
  int index = test.find(":");
  std::cout << test.substr(0, index) << std::endl;
  std::cout << test.substr(index, test.size()) << std::endl;
  std::atoi("123");
  //  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  //  http_server_thread_ = std::make_unique<std::thread>(&StartServer);
  //  http_server_thread_->detach();
  StartServer();
  //  while (true) {
  //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //  }
  return EXIT_SUCCESS;
}
