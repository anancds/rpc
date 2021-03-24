//
// Created by cds on 2020/10/15.
//
//
// Created by cds on 2020/9/29.
//
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <regex>
#include <thread>
#include <atomic>
#include "http_server.h"
#include "ps/core/thread_pool.h"

using namespace std;
using namespace mindspore;
using namespace mindspore::ps::core;

static atomic_uint64_t a = 0;

static ThreadPool pool(1);

void Process(std::shared_ptr<mindspore::ps::core::HttpMessageHandler> resp) {
  std::string host = resp->GetRequestHost();

  std::string path_param = resp->GetPathParam("key1");
  std::string header_param = resp->GetHeadParam("headerKey");
  std::string post_param = resp->GetPostParam("postKey");

  unsigned char *data = nullptr;
  const uint64_t len = resp->GetPostMsg(&data);
  char post_message[len + 1];
  memset_s(post_message, len + 1, 0, len + 1);
  if (memcpy_s(post_message, len, data, len) != 0) {
  }

  const std::string rKey("headKey");
  const std::string rVal("headValue");
  const std::string rBody("post request success!\n");
  resp->AddRespHeadParam(rKey, rVal);
  resp->AddRespString(rBody);

  resp->SetRespCode(200);
  resp->SendResponse();
  a++;
  MS_LOG(ERROR) << "the count is:" << a;
}

void testGetHandler(std::shared_ptr<mindspore::ps::core::HttpMessageHandler> resp) { pool.Submit(Process, resp); }

bool CheckIp(const std::string &ip) {
  std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
  std::smatch res;
  if (regex_match(ip, res, pattern)) {
    return true;
  }
  return false;
}
void StartHttpServer(HttpServer *server_) {
  mindspore::ps::core::OnRequestReceive f1 = std::bind(
    [](std::shared_ptr<mindspore::ps::core::HttpMessageHandler> resp) {
      const unsigned char ret[] = "get request success!\n";
      resp->QuickResponse(200, ret, 22);
    },
    std::placeholders::_1);
  server_->RegisterRoute("/httpget", &f1);
  mindspore::ps::core::OnRequestReceive f2 = std::bind(testGetHandler, std::placeholders::_1);
  server_->RegisterRoute("/handler", &f2);
  server_->Start();
}
int main() {
  std::int16_t test = -1;
  std::cout << test << std::endl;
  cout << CheckIp("0.0.0.0") << endl;
  HttpServer *server_ = new HttpServer("0.0.0.0", 9999);
  std::thread http_server_thread_([&]() { StartHttpServer(server_); });
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  http_server_thread_.join();

  return 0;
}