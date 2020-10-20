//
// Created by cds on 2020/10/16.
//

#include <stdlib.h>
#include <iostream>
#include <thread>
#include "tcp_server.h"

static void StartServer() {
  mindspore::ps::comm::TcpServer *server = new mindspore::ps::comm::TcpServer("127.0.0.1", 9000);
  server->ReceiveMessage([](const mindspore::ps::comm::TcpServer &server,
                            const mindspore::ps::comm::TcpConnection &conn, const void *buffer, size_t num) {
    // Dump message
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;

    // Send echo
    server.SendMessage(conn, buffer, num);
  });

  // Run on port 9000
  server->InitServer();

  server->Start();
}
int main(int /*argc*/, char ** /*argv*/) {
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_.reset(new std::thread(&StartServer));
  http_server_thread_->join();

  return EXIT_SUCCESS;
}
