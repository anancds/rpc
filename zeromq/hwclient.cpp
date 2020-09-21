//
// Created by cds on 2020/9/17.
//

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <zmq.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>

using namespace std;

class Config {
 public:
  static std::string repEndpoint;
  static std::string reqUniqueName;
  static uint64_t payloadSize;
  static int64_t executionTime;
  static int clients;
  static int threads;
};

std::string Config::repEndpoint;
std::string Config::reqUniqueName;
uint64_t Config::payloadSize;
int64_t Config::executionTime;
int Config::clients;
int Config::threads;
std::atomic_int64_t throughput = {0};

void sendData(int i) {
  //  Prepare our context and socket
  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);

  std::cout << "Connecting to hello world server..." << std::endl;
  socket.connect(Config::repEndpoint);

  std::string data(Config::payloadSize, 'a');
  float latency = 0.0f;
  std::size_t request_nbr = 0;
  std::chrono::time_point<std::chrono::high_resolution_clock> executionStartTime = chrono::high_resolution_clock::now();
  while (true) {
    if ((chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - executionStartTime)).count() >
        Config::executionTime) {
      cout << "thread" << i << ":the throughput is: " << request_nbr / Config::executionTime << endl;
      cout << "thread" << i << ":the latency is: " << latency / request_nbr << " milliseconds" << endl;
      break;
    }
    throughput++;
    request_nbr++;

    std::chrono::time_point<std::chrono::high_resolution_clock> startTime = chrono::high_resolution_clock::now();
    zmq::message_t request(Config::payloadSize);

    memcpy(request.data(), data.c_str(), Config::payloadSize);
    socket.send(request);

    //  Get the reply.
    zmq::message_t reply;
    socket.recv(&reply);
    chrono::duration<double, std::ratio<1, 1000>> duration_ms =
      chrono::duration_cast<chrono::duration<double, std::ratio<1, 1000>>>(chrono::high_resolution_clock::now() -
                                                                           startTime);
    latency += duration_ms.count();
  }
}

int main(int argc, char *argv[]) {
  boost::program_options::options_description description("Options");
  description.add_options()("help", "produce help message")(
    "PubEP", boost::program_options::value(&Config::repEndpoint)->default_value("tcp://localhost:5555"),
    "Publish endpoint (e.g. tcp://*:4242")(
    "pubUniqueName", boost::program_options::value(&Config::reqUniqueName)->default_value("req1"), "req Unique Name")(
    "msgLength", boost::program_options::value(&Config::payloadSize)->default_value(100000), "Message Length (bytes)")(
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

  thread clientThread[Config::clients];

  for (std::size_t i = 0; i < Config::clients; i++) {
    clientThread[i] = thread(sendData, i);
  }

  for (std::size_t i = 0; i < Config::clients; i++) {
    clientThread[i].join();
  }
  cout << "the total throughput is : " << throughput / Config::executionTime << endl;
//  cout << "the latency is: " << latency / throughput << " milliseconds" << endl;

  return 0;
}
