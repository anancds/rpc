#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else

#include "../build/helloworld.grpc.pb.h"

#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;
using namespace std;

class Config {
 public:
  static std::string gRPCEndpoint;
  static std::string reqUniqueName;
  static uint64_t payloadSize;
  static int64_t executionTime;
  static int clients;
  static int threads;
};

std::string Config::gRPCEndpoint;
std::string Config::reqUniqueName;
uint64_t Config::payloadSize;
int64_t Config::executionTime;
int Config::clients;
int Config::threads;
std::atomic_int64_t num;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel) : stub_(Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string &user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

void sendData() {
  GreeterClient greeter(grpc::CreateChannel(Config::gRPCEndpoint, grpc::InsecureChannelCredentials()));

  std::size_t request_nbr = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> executionStartTime = chrono::high_resolution_clock::now();
  while (true) {
    if ((chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - executionStartTime)).count() >
        Config::executionTime) {
      break;
    }
    request_nbr++;
    std::string user(Config::payloadSize, 'a');
    std::string reply = greeter.SayHello(user);
    std::cout << "Greeter received: " << reply << std::endl;

    chrono::duration<double, std::ratio<1, 1000>> duration_ms =
      chrono::duration_cast<chrono::duration<double, std::ratio<1, 1000>>>(chrono::high_resolution_clock::now() -
                                                                           executionStartTime);
    std::cout << duration_ms.count() << " milliseconds" << std::endl;
  }
}

int main(int argc, char **argv) {
  boost::program_options::options_description description("Options");
  description.add_options()("help", "produce help message")(
    "gRPC_srver", boost::program_options::value(&Config::gRPCEndpoint)->default_value("tcp://localhost:50051"),
    "Publish endpoint (e.g. tcp://localhost:50051")(
    "pubUniqueName", boost::program_options::value(&Config::reqUniqueName)->default_value("req1"), "req Unique Name")(
    "msgLength", boost::program_options::value(&Config::payloadSize)->default_value(100), "Message Length (bytes)")(
    "executionTime", boost::program_options::value(&Config::executionTime)->default_value(10),
    "Benchmark Execution Time (sec)")("threads", boost::program_options::value(&Config::threads)->default_value(1),
                                      "Number of threads per client")(
    "clients", boost::program_options::value(&Config::clients)->default_value(1), "Number of clients");
  boost::program_options::variables_map vm;

  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), vm);
    boost::program_options::notify(vm);
  } catch (const boost::program_options::error &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  if (vm.count("help")) {
    std::cout << description << "\n";
    return EXIT_SUCCESS;
  }

  std::thread clientThread[Config::threads];

  for (std::size_t i = 0; i < Config::threads; i++) {
    clientThread[i] = std::thread(sendData);
  }

  for (std::size_t i = 0; i < Config::threads; i++) {
    clientThread[i].join();
  }

  return 0;
}
