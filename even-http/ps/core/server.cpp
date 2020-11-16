//
// Created by cds on 2020/11/13.
//

#include <memory>
#include "ps/core/tcp_client.h"
#include "ps/core/tcp_server.h"

using namespace mindspore::ps::core;

int main(int /*argc*/, char ** /*argv*/) {
  std::unique_ptr<TcpServer> server_ = std::make_unique<TcpServer>("127.0.0.1", 9999);
  server_->SetMessageCallback([](const TcpServer &server, const TcpConnection &conn, const CommMessage &message) {
    KVMessage kv_message;
    kv_message.ParseFromString(message.data());
    std::cout << kv_message.keys_size() << std::endl;
    server.SendMessage(conn, message);
  });
  server_->Init();
  server_->Start();
}