//
// Created by cds on 2020/10/16.
//

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "message.h"
#include "tcp_server.h"
#include "../../../build/even-http/ps/comm/comm.pb.h"

using namespace mindspore::ps::comm;

static std::string getEnvVar(std::string const &key) {
  char *val = getenv(key.c_str());
  return val == NULL ? std::string("") : std::string(val);
}
static void StartServer() {
  TcpServer *server = new TcpServer("127.0.0.1", 9000);


  // Run on port 9000
  server->Init();

  server->Start();
}

int main(int /*argc*/, char ** /*argv*/) {
  std::cout << sizeof(unsigned char) << std::endl;
  uint32_t a = 1;
  std::cout << sizeof(a) << std::endl;
  std::cout << sizeof(uint32_t) << std::endl;

  MSLOG_IF(mindspore::EXCEPTION, true, mindspore::UnavailableError)<< "test";
  std::unique_ptr<unsigned char> message_buffer_(nullptr);
  message_buffer_.reset(new unsigned char [1]);
  //  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  //  http_server_thread_ = std::make_unique<std::thread>(&StartServer);
  //  http_server_thread_->detach();

  StartServer();
  return EXIT_SUCCESS;
}
