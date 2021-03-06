#include "proto_client.h"
#include <thread>
#include <iostream>
#include <stdlib.h>
int main(int /*argc*/, char** /*argv*/)
{
    const std::string test_message(5000, 's');

    // Run msg server
    proto::msgclient client;
    client.set_message_callback([&](proto::msgclient&, const void* buffer, size_t num)
    {
        std::cout << "Message received: " << std::string(reinterpret_cast<const char*>(buffer), num) << std::endl;
      client.send_msg(test_message.c_str(), test_message.size());
    });
  // Run on port 9000
    client.set_target("127.0.0.1:9000");
    client.start();

//  client.send_msg(test_message.c_str(), test_message.size());
    // Run for 5 minutes
    auto start_tp = std::chrono::steady_clock::now();

  client.send_msg(test_message.c_str(), test_message.size());

  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_ = std::make_unique<std::thread>([&](){

    client.update();
  });
  http_server_thread_->detach();
  client.start1();
  while (std::chrono::steady_clock::now() - start_tp < std::chrono::minutes(5))
    {
//      client.send_msg(test_message.c_str(), test_message.size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//        if (rand() % 100 < 10)
//            client.send_msg(test_message.c_str(), test_message.size());
    }
    return EXIT_SUCCESS;
}
