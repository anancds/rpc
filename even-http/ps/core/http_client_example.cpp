#include "ps/core/http_client.h"

using namespace mindspore::ps::core;

int main(int argc, char **argv) {
  mindspore::ps::core::HttpClient client;
  client.Init();
  int ret = 0;
  std::map<std::string, std::string> headers = {{"headerKey", "headerValue"}};
  
  auto output = std::make_shared<std::vector<char>>();
  ret =
    client.MakeRequest("http://www.baidu.com", mindspore::ps::core::HttpMethod::HM_GET, nullptr, 0, headers, output);
  MS_LOG(INFO) << output->data();
  MS_LOG(INFO) << "the code is:" << ret;

  auto output1 = std::make_shared<std::vector<char>>();
  ret = client.MakeRequest("http://127.0.0.1:9999/httpget", mindspore::ps::core::HttpMethod::HM_GET, nullptr, 0,
                           headers, output1);
  MS_LOG(INFO) << output1->data();
  MS_LOG(INFO) << "the code is:" << ret;

  auto output2 = std::make_shared<std::vector<char>>();
  std::string post_data = "postKey=postValue";
  ret = client.MakeRequest("http://127.0.0.1:9999/handler?key1=value1", mindspore::ps::core::HttpMethod::HM_POST,
                           post_data.c_str(), post_data.length(), headers, output2);
  MS_LOG(INFO) << output2->data();
  MS_LOG(INFO) << "the code is:" << ret;

  return 0;
}