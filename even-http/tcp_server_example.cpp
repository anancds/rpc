//
// Created by cds on 2020/10/16.
//

#include <stdlib.h>
#include <iostream>
#include <thread>
#include "tcp_server.h"

static std::string getEnvVar(std::string const &key) {
  char *val = getenv(key.c_str());
  return val == NULL ? std::string("") : std::string(val);
}
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
  struct evbuffer *buf1 = NULL, *buf2 = NULL;
  buf1 = evbuffer_new();
  buf2 = evbuffer_new();
  evbuffer_add_reference(buf1, "foo", 3, NULL, NULL);
  evbuffer_prepend(buf1, "abc", 3);
  int ret = evbuffer_remove_buffer(buf1, buf2, 1);
  std::cout << ret << std::endl;
//  evbuffer_add(buf1, "bar", 3);
  std:: cout<< evbuffer_pullup(buf1, -1) << std::endl;
  std::cout << static_cast<size_t>(-1) << std::endl;
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_.reset(new std::thread(&StartServer));
  http_server_thread_->join();

  return EXIT_SUCCESS;
}
