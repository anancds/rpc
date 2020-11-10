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
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "ps/core/http_server.h"

using namespace std;

namespace mindspore {
namespace ps {
namespace core {
static void testGetHandler(std::shared_ptr<HttpMessageHandler> resp) {
  std::string host = resp->GetRequestHost();
  EXPECT_STREQ(host.c_str(), "127.0.0.1");

  std::string path_param = resp->GetPathParam("key1");
  std::string header_param = resp->GetHeadParam("headerKey");
  std::string post_param = resp->GetPostParam("postKey");
  std::string post_message = resp->GetPostMsg();
  //    EXPECT_STREQ(path_param.c_str(), "value1");
  //    EXPECT_STREQ(header_param.c_str(), "headerValue");
  //    EXPECT_STREQ(post_param.c_str(), "postValue");
  //    EXPECT_STREQ(post_message.c_str(), "postKey=postValue");

  const std::string rKey("headKey");
  const std::string rVal("headValue");
  const std::string rBody("post request success!\n");
  resp->AddRespHeadParam(rKey, rVal);
  resp->AddRespString(rBody);

  resp->SetRespCode(200);
  resp->SendResponse();
}
class TestHttpServer : public ::testing::Test {
 protected:
  void SetUp() override {
    server_ = new HttpServer("0.0.0.0", 9999);
    OnRequestReceive http_get_func = std::bind(
      [](std::shared_ptr<HttpMessageHandler> resp) {
        EXPECT_STREQ(resp->GetPathParam("key1").c_str(), "value1");
        EXPECT_STREQ(resp->GetUriQuery().c_str(), "key1=value1");
        EXPECT_STREQ(resp->GetRequestUri().c_str(), "/httpget?key1=value1");
        EXPECT_STREQ(resp->GetUriPath().c_str(), "/httpget");
        resp->QuickResponse(200, "get request success!\n");

      },
      std::placeholders::_1);

    OnRequestReceive http_handler_func = std::bind(
      [](std::shared_ptr<HttpMessageHandler> resp) {
        std::string host = resp->GetRequestHost();
        EXPECT_STREQ(host.c_str(), "127.0.0.1");

        std::string path_param = resp->GetPathParam("key1");
        std::string header_param = resp->GetHeadParam("headerKey");
        std::string post_param = resp->GetPostParam("postKey");
        std::string post_message = resp->GetPostMsg();
        //    EXPECT_STREQ(path_param.c_str(), "value1");
        //    EXPECT_STREQ(header_param.c_str(), "headerValue");
        //    EXPECT_STREQ(post_param.c_str(), "postValue");
        //    EXPECT_STREQ(post_message.c_str(), "postKey=postValue");

        const std::string rKey("headKey");
        const std::string rVal("headValue");
        const std::string rBody("post request success!\n");
        resp->AddRespHeadParam(rKey, rVal);
        resp->AddRespString(rBody);

        resp->SetRespCode(200);
        resp->SendResponse();
      },
      std::placeholders::_1);
    server_->RegisterRoute("/httpget", &http_get_func);
    server_->RegisterRoute("/handler", &http_handler_func);
    std::unique_ptr<std::thread> http_server_thread_(nullptr);
    http_server_thread_ = std::make_unique<std::thread>([&]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      server_->Start();
    });
    http_server_thread_->detach();
  }

  void TearDown() override { server_->Stop(); }

 private:
  HttpServer *server_;
};

TEST_F(TestHttpServer, helloworld) {
  char buffer[100];
  FILE *file;
  std::string cmd = "curl -X GET http://127.0.0.1:9999/httpget?key1=value1";
  std::string result;
  const char *sysCommand = cmd.data();
  if ((file = popen(sysCommand, "r")) == nullptr) {
    return;
  }
  while (fgets(buffer, sizeof(buffer) - 1, file) != nullptr) {
    result += buffer;
  }
  EXPECT_STREQ("get request success!\n", result.c_str());
  pclose(file);
}

TEST_F(TestHttpServer, messageHandlerTest) {
  char buffer[100];
  FILE *file;
  std::string cmd =
    R"(curl -X POST -d 'postKey=postValue' -i -H "Accept: application/json" -H "headerKey: headerValue"  http://127.0.0.1:9999/handler?key1=value1)";
  std::string result;
  const char *sysCommand = cmd.data();
  if ((file = popen(sysCommand, "r")) == nullptr) {
    return;
  }
  while (fgets(buffer, sizeof(buffer) - 1, file) != nullptr) {
    result += buffer;
  }
  EXPECT_STREQ("post request success!\n", result.substr(result.find("post")).c_str());
  pclose(file);
}

TEST_F(TestHttpServer, portErrorNoException) {
  HttpServer *server_exception = new HttpServer("0.0.0.0", -1);
  OnRequestReceive http_handler_func = std::bind(testGetHandler, std::placeholders::_1);
  EXPECT_NO_THROW(server_exception->RegisterRoute("/handler", &http_handler_func));
}

TEST_F(TestHttpServer, addressException) {
  HttpServer *server_exception = new HttpServer("12344.0.0.0", 9998);
  OnRequestReceive http_handler_func = std::bind(testGetHandler, std::placeholders::_1);
  ASSERT_THROW(server_exception->RegisterRoute("/handler", &http_handler_func), std::exception);
}
}  // namespace core
}  // namespace ps

}  // namespace mindspore
