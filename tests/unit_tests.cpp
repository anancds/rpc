//
// Created by cds on 2020/10/9.
//
#include <grpc/grpc.h>
#include <grpc/grpc_security.h>
#include <grpc/support/alloc.h>
#include <grpc/support/log.h>
#include <grpc/support/string_util.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "event_http_server.h"

using namespace std;
namespace Network {

class TestHttpServer : public ::testing::Test {
 protected:
  static void testGetHandler(EvHttpResp *resp) {
    std::string strHost = resp->GetRequestHost();
    int nPort = resp->GetUriPort();
    EXPECT_STREQ(strHost.c_str(), "127.0.0.1");
    std::string strPath = resp->GetUriPath();
    std::string strQuery = resp->GetUriQuery();
    std::cout << "Get host:" << strHost << std::endl;
    std::cout << "Get port:" << nPort << std::endl;
    std::cout << "Get uri:" << strPath << std::endl;
    std::cout << "Get uri params:" << strQuery << std::endl;

    const std::string gKey("key1");
    const std::string hKey("header");
    const std::string pKey("hello");
    std::string strGVal = resp->GetPathParam(gKey);
    std::string strHVal = resp->GetHeadParam(hKey);
    std::string strPVal = resp->GetPostParam(pKey);
    std::string post_message = resp->GetPostMsg();
    std::cout << "Get path param:" << strGVal << std::endl;
    std::cout << "Get head param:" << strHVal << std::endl;
    std::cout << "Get post param:" << strPVal << std::endl;
    std::cout << "Get post message:" << post_message << std::endl;

    std::string strBody = resp->GetPostMsg();

    const std::string rKey("retHead");
    const std::string rVal("retValue");
    const std::string rBody("Hello World!\n");
    resp->AddRespHeadParam(rKey, rVal);
    resp->AddRespString(rBody);
//    const std::string rBuf("Hello World is over!\n");
//    resp->AddRespBuf(rBuf.c_str(), rBuf.length());

    resp->SetRespCode(200);
    resp->SendResponse();
  }
  void SetUp() override {
    server_ = new Network::EvHttpServ("0.0.0.0", 8077);
    server_->RegistHandler("/hello", [](Network::EvHttpResp *resp) {
      std::cout << resp->GetPathParam("key1") << std::endl;
      std::cout << resp->GetUriQuery() << std::endl;
      std::cout << resp->GetUriFragment() << std::endl;
      std::cout << resp->GetRequestUri() << std::endl;
      std::cout << resp->GetUriPath() << std::endl;
      resp->QuickResponse(200, "Hello World!\n");
    });
    server_->RegistHandler("/handler", TestHttpServer::testGetHandler);
    std::unique_ptr<std::thread> http_server_thread_(nullptr);
    http_server_thread_ = std::make_unique<std::thread>([&]() { server_->Start(); });
    http_server_thread_->detach();
  }

  void TearDown() override { server_->Stop(); }

  Network::EvHttpServ *server_;
};

TEST_F(TestHttpServer, helloworld) {
  char buffer[100];
  FILE *file;
  std::string cmd = "curl -X GET http://127.0.0.1:8077/hello?key1=value1";
  std::string result;
  const char *sysCommand = cmd.data();
  if ((file = popen(sysCommand, "r")) == nullptr) {
    cout << "error" << endl;
    return;
  }
  while (fgets(buffer, sizeof(buffer) - 1, file) != nullptr) {
    result += buffer;
  }
  EXPECT_STREQ("Hello World!\n", result.c_str());
  pclose(file);
}

TEST_F(TestHttpServer, messageHandlerTest) {
  char buffer[100];
  FILE *file;
  std::string cmd =
    R"(curl -X POST -d 'hello=world' -i -H "Accept: application/json" -H "header: header" http://127.0.0.1:8077/handler?key1=value1)";
  std::string result;
  const char *sysCommand = cmd.data();
  if ((file = popen(sysCommand, "r")) == nullptr) {
    cout << "error" << endl;
    return;
  }
  while (fgets(buffer, sizeof(buffer) - 1, file) != nullptr) {
    result += buffer;
  }
  EXPECT_STREQ("Hello World!\n", result.substr(result.find("Hello")).c_str());
  pclose(file);
}
}  // namespace Network
