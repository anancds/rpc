
#include <stdlib.h>
#include <iostream>
#include <memory>
#include <thread>
#include "ps/core/node_manager_test.h"

#include "../../../build/even-http/ps/core/comm.pb.h"
#include "../../../build/even-http/ps/core/ps.pb.h"

using namespace mindspore::ps::core;
using namespace mindspore::ps;

const std::string test_message(1024, 's');
using Key = uint64_t;
static void StartClient(mindspore::ps::core::TcpClient *client) {
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

static void Start() {
  ClusterConfig::Init(0, 1, std::make_unique<std::string>("127.0.0.1"), 9999);
  NodeManagerTest::Get()->StartServer();
}

int main(int /*argc*/, char ** /*argv*/) {
  Start();

  return EXIT_SUCCESS;
}
