#include <zmq.hpp>
#include <string>
#include <iostream>

#ifndef _WIN32

#include <unistd.h>

#else
#include <windows.h>

#define sleep(n)	Sleep(n)
#endif


#include <chrono>
#include "zhelpers.hpp"
#include "nlohmann/json.hpp"
#include "BenchmarkLogger.hpp"

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

int main() {
    //  Prepare our context and socket
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:5555");

    while (true) {
        zmq::message_t message;
        socket.recv(&message);
        high_resolution_clock::time_point p = high_resolution_clock::now();
        microseconds nowTime = duration_cast<microseconds>(p.time_since_epoch());


        auto msg = std::string(static_cast<char *>(message.data()), message.size());
        cout << msg << endl;
//        std::vector<float> *v1 = static_cast<std::vector<float>*>(message.data());
//         cout << (*v1).size() << endl;
        //char *msg = static_cast<char*>(message.data());
//        json json_msg = json::parse(msg);

//
//        auto srcTimestamp = std::string(json_msg["time_stamp"]);
//        if (srcTimestamp.compare("0") == 0)
//        {
//            cout << "END OF TEST" << endl;
//            BenchmarkLogger::DumpResultsToFile();
//            break;
//        }
//        else
//        {
//            auto microsecLatency = nowTime.count() - std::stoll(srcTimestamp);
//            //cout << "One way latency (millisec) = " << static_cast<double>(microsecLatency) / 1000 << endl;
//            BenchmarkLogger::latencyResults.emplace_back(
//                    json_msg["src"],
//                    json_msg["seq_num"],
//                    microsecLatency);
//        }

        zmq::message_t reply(0);
        memcpy(reply.data(), "", 0);
        socket.send(reply);
    }
    return 0;
}