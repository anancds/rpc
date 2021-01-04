//
// Created by cds on 2020/11/13.
//

#include <memory>
#include <thread>
#include "ps/core/tcp_client.h"

using namespace mindspore::ps::core;
using namespace mindspore::ps;

int main(int /*argc*/, char ** /*argv*/) {
  std::unique_ptr<TcpClient> client_ = std::make_unique<TcpClient>("127.0.0.1", 9999);
  client_->SetMessageCallback([&](const TcpClient &client, const CommMessage &message) {
    KVMessage kv_message;
    kv_message.ParseFromString(message.data());
    std::cout << kv_message.keys_size() << std::endl;
    client.SendMessage(message);
  });

  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  client_->Init();
  http_server_thread_ = std::make_unique<std::thread>([&]() { client_->Start(); });

  client_->set_disconnected_callback([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(ClusterConfig::connect_interval()));
    client_->Init();
  });
  if (!client_->WaitConnected(10)) {
    MS_LOG(EXCEPTION) << "Connected failed!";
  }
  CommMessage comm_message;
  KVMessage kv_message;
  std::vector<int> keys{1, 2};
  std::vector<int> values{3, 4};
  *kv_message.mutable_keys() = {keys.begin(), keys.end()};
  *kv_message.mutable_values() = {values.begin(), values.end()};
  kv_message.set_command(PSCommand::PUSH);

  comm_message.set_data(kv_message.SerializeAsString());
  client_->SendMessage(comm_message);
  std::this_thread::sleep_for(std::chrono::milliseconds(500000));
    client_->Stop();
  http_server_thread_->join();
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  return EXIT_SUCCESS;
}