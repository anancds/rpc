#include <boost/asio.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <memory>
#include <thread>

using boost::asio::ip::tcp;
using namespace std;
using namespace chrono;

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

enum { max_length = 10240000 };

void sendData(int i) {
  try {
    std::cout << "connecting to server" << std::endl;
    boost::asio::io_context io_context;

    tcp::socket s(io_context);
    tcp::resolver resolver(io_context);
    boost::asio::connect(s, resolver.resolve("127.0.0.1", "9999"));

    std::size_t request_nbr = 0;

    std::chrono::time_point<std::chrono::high_resolution_clock> executionStartTime =
      chrono::high_resolution_clock::now();
    std::string request(Config::payloadSize, 'a');
    float latency = 0.0f;
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
      boost::asio::write(s, boost::asio::buffer(request, max_length));
      char reply[max_length];
      size_t reply_length = boost::asio::read(s, boost::asio::buffer(reply, Config::payloadSize));

      chrono::duration<double, std::ratio<1, 1000>> duration_ms =
        chrono::duration_cast<chrono::duration<double, std::ratio<1, 1000>>>(chrono::high_resolution_clock::now() -
                                                                             startTime);
      latency += duration_ms.count();
    }
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
}

int main(int argc, char *argv[]) {
  boost::program_options::options_description description("Options");
  description.add_options()("help", "produce help message")(
    "gRPC_srver", boost::program_options::value(&Config::gRPCEndpoint)->default_value("localhost:50051"),
    "Publish endpoint (e.g. localhost:50051")(
    "pubUniqueName", boost::program_options::value(&Config::reqUniqueName)->default_value("req1"), "req Unique Name")(
    "msgLength", boost::program_options::value(&Config::payloadSize)->default_value(10000000), "Message Length (bytes)")(
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
