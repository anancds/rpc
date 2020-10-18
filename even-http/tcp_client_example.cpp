
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "tcp_client.h"
const std::string test_message = "TEST_MESSAGE";
static void StartClient(mindspore::ps::comm::TcpClient *client) {
  // Run msg server
  client->set_message_callback([](mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
    client.send_msg(buffer, num);
  });

  // Run on port 9000
  client->set_target("127.0.0.1:9000");
  client->InitTcpClient();

  // Run for 5 minutes
  auto start_tp = std::chrono::steady_clock::now();

  client->send_msg(test_message.c_str(), test_message.size());
  client->update();
}
int main(int /*argc*/, char ** /*argv*/) {
  mindspore::ps::comm::TcpClient client;
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_ = std::make_unique<std::thread>(&StartClient, &client);
  //  client.send_msg(test_message.c_str(), test_message.size());
  http_server_thread_->join();

  return EXIT_SUCCESS;
}
