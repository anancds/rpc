//
// Created by cds on 2020/10/16.
//
#include <stdlib.h>
#include <iostream>
#include <thread>
#include "proto_server.h"
#include "utils/log_adapter.h"

static void StartServer() {
  proto::msgserver server;
  server.set_msg_callback([](proto::msgserver &server, proto::msgconnection &conn, const void *buffer, size_t num) {
    // Dump message
    std::cout << "Message received: " << std::string(reinterpret_cast<const char *>(buffer), num) << std::endl;

    // Send echo
    server.send_msg(conn, buffer, num);
  });

  // Run on port 9000
  server.setup(9000);

  server.update();
}
int main(int /*argc*/, char ** /*argv*/) {
  MS_LOG(INFO) << "Init http server failed!";
  // Run msg server

  // Run for 5 minutes
  auto start_tp = std::chrono::steady_clock::now();

  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_.reset(new std::thread(&StartServer));
  http_server_thread_->join();
  //  while (std::chrono::steady_clock::now() - start_tp < std::chrono::minutes(1))
  //    {
  //        server.update();
  //        std::this_thread::sleep_for(std::chrono::milliseconds(1));
  //    }
  return EXIT_SUCCESS;
}
