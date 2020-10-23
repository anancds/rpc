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
#include "http_server.h"

using namespace std;
using namespace mindspore;

static void testGetHandler(std::shared_ptr<mindspore::ps::comm::HttpMessageHandler> resp) {
  std::string host = resp->GetRequestHost();

  std::string path_param = resp->GetPathParam("key1");
  std::string header_param = resp->GetHeadParam("headerKey");
  std::string post_param = resp->GetPostParam("postKey");
  std::string post_message = resp->GetPostMsg();

  const std::string rKey("headKey");
  const std::string rVal("headValue");
  const std::string rBody("post request success!\n");
  resp->AddRespHeadParam(rKey, rVal);
  resp->AddRespString(rBody);

  resp->SetRespCode(200);
  resp->SendResponse();
}

bool CheckIp(const std::string &ip) {
  std::regex pattern("((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)");
  std::smatch res;
  if (regex_match(ip, res, pattern)) {
    return true;
  }
  return false;
}
void StartHttpServer() {
  mindspore::ps::comm::HttpServer *server_ = new mindspore::ps::comm::HttpServer("0.0.0.0", 9999);
  mindspore::ps::comm::HandlerFunc f1 = std::bind([](std::shared_ptr<mindspore::ps::comm::HttpMessageHandler> resp) {
    resp->QuickResponse(200, "get request success!\n");
  }, std::placeholders::_1);
  server_->RegisterRoute("/httpget", &f1);
  mindspore::ps::comm::HandlerFunc f2 = std::bind(testGetHandler, std::placeholders::_1);
  server_->RegisterRoute("/handler", &f2);
  server_->Start();
}
int main() {
  std::int16_t test = -1;
  std::cout << test << std::endl;
  cout << CheckIp("0.0.0.0") << endl;
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_ = std::make_unique<std::thread>(&StartHttpServer);
  http_server_thread_->join();
  return 0;
}