
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "tcp_client.h"
#include "../../../build/even-http/ps/comm/comm.pb.h"
#include "../../../build/even-http/ps/comm/ps.pb.h"

using namespace mindspore::ps::comm;
using namespace mindspore::ps;

const std::string test_message(1024, 's');
using Key = uint64_t;
static void StartClient(mindspore::ps::comm::TcpClient *client) {
  // Run msg server
  std::unique_ptr<std::thread> http_client_thread(nullptr);
  http_client_thread = std::make_unique<std::thread>([&]() {
    client->SetMessageCallback([](const TcpClient &client, const CommMessage &message) {
      KVMessage kv_message;
      kv_message.ParseFromString(message.data());
    });

    client->Init();

    CommMessage comm_message;
    KVMessage kv_message;
    std::vector<int> keys{1, 2};
    std::vector<int> values{3, 4};
    *kv_message.mutable_keys() = {keys.begin(), keys.end()};
    *kv_message.mutable_values() = {values.begin(), values.end()};

    comm_message.set_data(kv_message.SerializeAsString());
    client->SendMessage(comm_message);

    client->Start();
  });
}

int main(int /*argc*/, char ** /*argv*/) {
  auto client = new TcpClient("127.0.0.1", 9000);
  std::unique_ptr<std::thread> http_server_thread_(nullptr);
  http_server_thread_ = std::make_unique<std::thread>(&StartClient, client);
  //  client.send_msg(test_message.c_str(), test_message.size());
  http_server_thread_->join();

  return EXIT_SUCCESS;
}
