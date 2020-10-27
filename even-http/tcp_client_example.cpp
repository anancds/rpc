
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "tcp_client.h"
const std::string test_message(5000, 's');
using Key = uint64_t;
static void StartClient(mindspore::ps::comm::TcpClient *client) {
  // Run msg server
  client->ReceiveMessage([](const mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
//    client.SendMessage(buffer, num);
  });

  // Run on port 9000
  client->InitTcpClient();

  // Run for 5 minutes
  auto start_tp = std::chrono::steady_clock::now();

  client->SendMessage(test_message.c_str(), test_message.size());
  client->Start();
}

static void StartClient1(mindspore::ps::comm::TcpClient *client) {
  // Run msg server
  client->ReceiveMessage([](const mindspore::ps::comm::TcpClient &client, const void *buffer, size_t num) {
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
//    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;
//    client.SendMessage(buffer, num);
  });

  // Run on port 9000
  client->InitTcpClient();

  // Run for 5 minutes
  auto start_tp = std::chrono::steady_clock::now();

//  Message message{};
//  int num = 10;
//  std::vector<uint64_t> keys(num);
//  std::vector<float> vals(num);
//
//  for (int i = 0; i < num; ++i) {
//    keys[i] = (rand() % 1000);
//    vals[i] = (rand() % 1000);
//  }
//  message.AddVectorData(keys, vals);

  Message message{};
  int num = 10;
  uint64_t keys[num];
  float vals[num];

  for (int i = 0; i < num; ++i) {
    keys[i] = (rand() % 1000);
    vals[i] = (rand() % 1000);
  }
  message.AddArrayData(keys, vals, 10, 10);

//  client->SendKVMessage(message);
  client->SendMessage(test_message.c_str(), test_message.size());
  client->Start();
}

int main(int /*argc*/, char ** /*argv*/) {
  mindspore::ps::comm::TcpClient *client = new mindspore::ps::comm::TcpClient("127.0.0.1", 9000);
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_ = std::make_unique<std::thread>(&StartClient, client);
  //  client.send_msg(test_message.c_str(), test_message.size());
  http_server_thread_->join();

  return EXIT_SUCCESS;
}
