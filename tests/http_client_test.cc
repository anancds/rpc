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
#include "ps/core/http_client.h"

using namespace std;

namespace mindspore {
namespace ps {
namespace core {
static void testGetHandler(std::shared_ptr<HttpMessageHandler> resp) {
  const std::string rKey("headKey");
  const std::string rVal("headValue");
  const std::string rBody("post request success!\n");
  resp->AddRespHeadParam(rKey, rVal);
  resp->AddRespString(rBody);

  resp->SetRespCode(200);
  resp->SendResponse();
}
class TestHttpClient : public ::testing::Test {
 protected:
  void SetUp() override {
    MS_LOG(INFO) << "Start http server!";
    server_ = new HttpServer("0.0.0.0", 9999);
    OnRequestReceive http_get_func =
      std::bind([](std::shared_ptr<HttpMessageHandler> resp) { resp->QuickResponse(200, "get request success!\n"); },
                std::placeholders::_1);

    OnRequestReceive http_handler_func = std::bind(
      [](std::shared_ptr<HttpMessageHandler> resp) {
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
    http_server_thread_ = std::make_unique<std::thread>([&]() { server_->Start(); });
    http_server_thread_->detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  void TearDown() override {
    server_->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  }

 private:
  HttpServer *server_;
};

TEST_F(TestHttpClient, helloworld) {
  HttpClient client;
  client.Init();
  std::map<std::string, std::string> headers = {{"headerKey", "headerValue"}};
  auto output = std::make_shared<std::vector<char>>();
  int ret = client.MakeRequest("http://127.0.0.1:9999/httpget", mindspore::ps::core::HttpMethod::HM_GET, nullptr, 0,
                               headers, output);
  EXPECT_STREQ("get request success!\n", output->data());
  EXPECT_EQ(200, ret);
}

TEST_F(TestHttpClient, messageHandlerTest) {
  HttpClient client;
  client.Init();
  std::map<std::string, std::string> headers = {{"headerKey", "headerValue"}};
  auto output = std::make_shared<std::vector<char>>();
  std::string post_data = "postKey=postValue";
  int ret = client.MakeRequest("http://127.0.0.1:9999/handler?key1=value1", mindspore::ps::core::HttpMethod::HM_POST,
                               post_data.c_str(), post_data.length(), headers, output);
  EXPECT_STREQ("post request success!\n", output->data());
  EXPECT_EQ(200, ret)
}
}  // namespace core
}  // namespace ps

}  // namespace mindspore
