#include "proto_server.h"
#include <thread>
#include <iostream>
#include <stdlib.h>

int main(int /*argc*/, char** /*argv*/)
{
    // Run msg server
    proto::msgserver server;
    server.set_msg_callback([](proto::msgserver& server, proto::msgconnection& conn, const void* buffer, size_t num)
    {
        // Dump message
        std::cout << "Message received: " << std::string(reinterpret_cast<const char*>(buffer), num) << std::endl;

        // Send echo
        server.send_msg(conn, buffer, num);
    });

    // Run on port 9000
    server.setup(9000);

    // Run for 5 minutes
    auto start_tp = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start_tp < std::chrono::minutes(5))
    {
        server.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return EXIT_SUCCESS;
}
