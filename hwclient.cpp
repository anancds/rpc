//
// Created by cds on 2020/9/17.
//

#include <zmq.hpp>
#include <string>
#include <iostream>

#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>

using namespace std;
class Config
{
public:
    static std::string pubEndpoint;
    static std::string publisherUniqueName;
    static uint64_t payloadSize;
    static int64_t executionTime;
    static double pubRate;
};

std::string Config::pubEndpoint;
std::string Config::publisherUniqueName;
uint64_t Config::payloadSize;
int64_t Config::executionTime;
double Config::pubRate;
int main(int argc, char *argv[]) {
    std::string a(14344324, '0');
    cout << sizeof(a) << endl;
    boost::program_options::options_description description("Options");
    description.add_options()("help", "produce help message")
            ("PubEP", boost::program_options::value(&Config::pubEndpoint)->default_value("ipc:///tmp/0"),
             "Publish endpoint (e.g. ipc:///tmp/0 or tcp://*:4242")
            ("pubUniqueName", boost::program_options::value(&Config::publisherUniqueName)->default_value("pub1"), "Publisher Unique Name")
            ("msgLength", boost::program_options::value(&Config::payloadSize)->default_value(100), "Message Length (bytes)")
            ("executionTime", boost::program_options::value(&Config::executionTime)->default_value(100), "Benchmark Execution Time (sec)")
            ("pubRate", boost::program_options::value(&Config::pubRate)->default_value(100), "Publishing rate");
    boost::program_options::variables_map vm;

    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, description), vm);
        boost::program_options::notify(vm);
    }
    catch (const boost::program_options::error &e)
    {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    if (vm.count("help"))
    {
        std::cout << description << "\n";
        return EXIT_SUCCESS;
    }
    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REQ);

    std::cout << "Connecting to hello world server..." << std::endl;
    socket.connect("tcp://localhost:5555");
    constexpr int num = 1000;
    char *data = new char[num];

    for (size_t i = 0; i < num; i++) {
        data[i] = 'a';
    }
    //  Do 10 requests, waiting each time for a response
    for (int request_nbr = 0; request_nbr != 10; request_nbr++) {

        zmq::message_t request(num);

        memcpy(request.data(), data, num);
        std::cout << "Sending Hello " << request_nbr << "..." << std::endl;
        std::cout << "size:" << strlen(data) << std::endl;
        socket.send(request);



        //  Get the reply.
        zmq::message_t reply;
        socket.recv(&reply);
        std::cout << "Received World " << request_nbr << std::endl;
    }

    delete[]data;
    return 0;
}
