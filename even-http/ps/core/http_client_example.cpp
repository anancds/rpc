#include "ps/core/http_client.h"

using namespace mindspore::ps::core;

int main(int argc, char **argv) {
  auto test = std::make_shared<std::vector<char>>();
  std::cout << test->data() << std::endl;
  mindspore::ps::core::HttpClient client;
  client.Init();
  Status ret;
  std::map<std::string, std::string> headers = {{"headerKey", "headerValue"}};

  // while (true) {
  //   auto output = std::make_shared<std::vector<char>>();
  //   ret = client.Get("http://www.baidu.com", output, headers);
  //   // MS_LOG(INFO) << output->data();
  //   MS_LOG(INFO) << "the code is:" << ret;
  // }

  while (true) {
    auto output1 = std::make_shared<std::vector<char>>();
    ret = client.Get("http://127.0.0.1:9999/httpget", output1, headers);
    MS_LOG(INFO) << output1->data();
    MS_LOG(INFO) << "the code is:" << ret;
  }

  // while (true) {
  //   // std::this_thread::sleep_for(std::chrono::milliseconds(1));
  //   auto output2 = std::make_shared<std::vector<char>>();
  //   std::string post_data = "postKey=postValue";
  //   ret =
  //     client.Post("http://127.0.0.1:9999/handler?key1=value1", post_data.c_str(), post_data.length(), output2,
  //     headers);
  //   MS_LOG(INFO) << output2->data();
  //   MS_LOG(INFO) << "the code is:" << ret;
  // }

  return 0;
}