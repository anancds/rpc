//
// Created by cds on 2020/11/13.
//

#include <memory>
#include "ps/core/tcp_client.h"
#include "ps/core/tcp_server.h"

using namespace mindspore::ps::core;
using namespace mindspore::ps;

int main(int /*argc*/, char ** /*argv*/) {
  std::shared_ptr<TcpServer> server_ = std::make_shared<TcpServer>("127.0.0.1", 9999);
  server_->SetMessageCallback(
    [&](std::shared_ptr<TcpConnection> conn, std::shared_ptr<CommMessage> message) {
      KVMessage kv_message;
      kv_message.ParseFromString(message->data());
      std::cout << kv_message.keys_size() << std::endl;
      server_->SendMessage(conn, message);
    });
  server_->Init();
  std::thread server_thread = std::thread([&]() { server_->Start(); });
  std::this_thread::sleep_for(std::chrono::seconds(5));
  //  server_->Stop();
  server_thread.join();
  std::this_thread::sleep_for(std::chrono::seconds(4));
}