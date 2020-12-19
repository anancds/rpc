//
// Created by cds on 2020/10/16.
//

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "../../../build/even-http/ps/core/comm.pb.h"
#include "ps/core/node_manager_test.h"
#include "tcp_server.h"
#include "ps/core/node_info.h"

using namespace mindspore::ps::core;

static void StartServer() {
  ClusterConfig::Init(1, 2, "127.0.0.1", 9999);
  NodeManagerTest::Get()->StartScheduler();
}

void test() {
  std::unordered_map<int, int> map;
  map.insert(std::make_pair(1, 1));
  uint32_t a = 2;
  if (map.size() == a) {
  }
}

void testUUID() { std::cout << CommUtil::GenerateUUID() << std::endl; }

void testMap() {
  std::unordered_map<int, std::unordered_map<int, int>> map;
  auto it = map.find(1);
  if (it != map.end()) {
    it->second.insert(std::make_pair(2, 2));
  } else {
    std::unordered_map<int, int> res;
    res.insert(std::make_pair(1, 1));
    map[1] = res;
  }
  std::cout << map[1].size() << std::endl;

  auto it1 = map.find(1);
  if (it1 != map.end()) {
    it1->second.insert(std::make_pair(2, 2));
  } else {
    it1->second.insert(std::make_pair(3, 3));
  }
  std::cout << map[1].size() << std::endl;
}

void test_size_t() {
  size_t size = 408;
  char res[100] = {0};
  memcpy_s(res, 4,&size, 4);
  memcpy_s(res + 4, 4,&size + 4, 4);
  size_t *test = reinterpret_cast<size_t *>(res);
  std::cout << *test << std::endl;
  std::cout << *reinterpret_cast<size_t *>(res) << std::endl;

  memcpy_s(res, 4,&size +4, 4);

}

int main(int /*argc*/, char ** /*argv*/) {

//  test_size_t();
//  testMap();
std::cout << sizeof(int) << std::endl;
std::cout << sizeof(float) << std::endl;
  StartServer();
  CommMessage message;
  //  while (true) {
  //    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //  }
  return EXIT_SUCCESS;
}
