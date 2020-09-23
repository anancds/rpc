#include <brpc/channel.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <gflags/gflags.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include "build/echo.pb.h"

using namespace std;

DEFINE_string(protocol, "h2:grpc", "Protocol type. Defined in src/brpc/options.proto");
DEFINE_string(server, "0.0.0.0:50051", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_int32(interval_ms, 1000, "Milliseconds between consecutive requests");
DEFINE_bool(gzip, false, "compress body using gzip");

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
std::atomic_int64_t throughput = {0};

void sendData(int i) {
  // A Channel represents a communication line to a Server. Notice that
  // Channel is thread-safe and can be shared by all threads in your program.
  brpc::Channel channel;

  // Initialize the channel, NULL means using default options.
  brpc::ChannelOptions options;
  options.protocol = FLAGS_protocol;
  options.timeout_ms = FLAGS_timeout_ms /*milliseconds*/;
  options.max_retry = FLAGS_max_retry;
  if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
    LOG(ERROR) << "Fail to initialize channel";
    return;
  }

  // Normally, you should not call a Channel directly, but instead construct
  // a stub Service wrapping it. stub can be shared by all threads as well.
  helloworld::Greeter_Stub stub(&channel);
  std::string user(Config::payloadSize, 'a');
  std::size_t request_nbr = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> executionStartTime = chrono::high_resolution_clock::now();
  float latency = 0.0f;
  while (true) {
    if ((chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - executionStartTime)).count() >
        Config::executionTime) {
      cout << "thread" << i << ":the throughput is: " << request_nbr / Config::executionTime << endl;
      cout << "thread" << i << ":the latency is: " << latency / request_nbr << " milliseconds" << endl;
      break;
    }
    helloworld::HelloRequest request;
    helloworld::HelloReply response;
    brpc::Controller cntl;
    request.set_name(user);
    if (FLAGS_gzip) {
      cntl.set_request_compress_type(brpc::COMPRESS_TYPE_GZIP);
    }
    throughput++;
    request_nbr++;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime = chrono::high_resolution_clock::now();
    stub.SayHello(&cntl, &request, &response, NULL);
    if (!cntl.Failed()) {
//      LOG(INFO) << "Received response from " << cntl.remote_side() << " to " << cntl.local_side() << ": "
//                << response.message() << " latency=" << cntl.latency_us() << "us";
    } else {
      LOG(WARNING) << cntl.ErrorText();
    }

    chrono::duration<double, std::ratio<1, 1000>> duration_ms =
    chrono::duration_cast<chrono::duration<double, std::ratio<1, 1000>>>(chrono::high_resolution_clock::now() -
                                                                         startTime);
    latency += duration_ms.count();
  }
}

int main(int argc, char *argv[]) {


  boost::program_options::options_description description("Options");
  description.add_options()("help", "produce help message")(
    "gRPC_srver", boost::program_options::value(&Config::gRPCEndpoint)->default_value("localhost:50051"),
    "Publish endpoint (e.g. localhost:50051")(
    "pubUniqueName", boost::program_options::value(&Config::reqUniqueName)->default_value("req1"), "req Unique Name")(
    "msgLength", boost::program_options::value(&Config::payloadSize)->default_value(100000),
    "Message Length (bytes)")("executionTime", boost::program_options::value(&Config::executionTime)->default_value(10),
                              "Benchmark Execution Time (sec)")(
    "threads", boost::program_options::value(&Config::threads)->default_value(1), "Number of threads per client")(
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

  std::thread clientThread[Config::clients];

  for (std::size_t i = 0; i < Config::clients; i++) {
    clientThread[i] = std::thread(sendData, i);
  }

  for (std::size_t i = 0; i < Config::clients; i++) {
    clientThread[i].join();
  }

  cout << "the total throughput is : " << throughput / Config::executionTime << endl;

  return 0;
}