//
// Created by cds on 2020/10/9.
//
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include "event_http_server.h"

using namespace std;
namespace Network {

class TestHttpServer : public ::testing::Test {
 protected:
  void SetUp() override {
    server_ = new Network::EvHttpServ("0.0.0.0", 8077);
    server_->RegistHandler("/hello", [](Network::EvHttpResp *resp) { resp->QuickResponse(200, "Hello World!\n"); });
  }

  void TearDown() override { server_->Stop(); }

  Network::EvHttpServ *server_;
};

TEST_F(TestHttpServer, helloworld) {
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_.reset(new std::thread([&]() {
    server_->Start();
    std::cout << "hello" << std::endl;
  }));
  http_server_thread_->detach();
  char line[100];
  FILE *file;
  std::string cmd = "curl -X GET http://127.0.0.1:8077/hello";
  std::string result;
  const char *sysCommand = cmd.data();
  if ((file = popen(sysCommand, "r")) == nullptr) {
    cout << "error" << endl;
    return;
  }
  while (fgets(line, sizeof(line) - 1, file) != nullptr) {
    result += line;
  }
  std::cout << result << std::endl;
  pclose(file);
}
}  // namespace Network
