//
// Created by cds on 2020/10/16.
//

#include "tcp_server.h"
#include <thread>
#include <iostream>
#include <stdlib.h>

static void StartServer() {
  mindspore::ps::comm::TcpServer server;
  server.SetMessageCallback([](mindspore::ps::comm::TcpServer &server, mindspore::ps::comm::TcpConnection &conn, const void *buffer, size_t num) {
    // Dump message
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;

    // Send echo
    server.SendMessage(conn, buffer, num);
  });

  // Run on port 9000
  server.InitServer(9000);

  server.Start();
}
int main(int /*argc*/, char** /*argv*/) {


  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_.reset(new std::thread(&StartServer));
  http_server_thread_->join();

  return EXIT_SUCCESS;
}
