
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "tcp_client.h"
const std::string test_message = "TEST_MESSAGE";
static void StartClient(mindspore::ps::comm::TcpClient *client) {
  // Run msg server
  client->SetMessageCallback([](mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
    client.SendMessage(buffer, num);
  });

  // Run on port 9000
  client->SetTarget("127.0.0.1:900");
  client->InitTcpClient();

  // Run for 5 minutes
  auto start_tp = std::chrono::steady_clock::now();

  client->SendMessage(test_message.c_str(), test_message.size());
  client->Start();
}
int main(int /*argc*/, char ** /*argv*/) {
  mindspore::ps::comm::TcpClient client;
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_ = std::make_unique<std::thread>(&StartClient, &client);
  //  client.send_msg(test_message.c_str(), test_message.size());
  http_server_thread_->join();

  return EXIT_SUCCESS;
}
