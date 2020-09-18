//
// Created by cds on 2020/9/17.
//

#include <zmq.hpp>
#include <string>
#include <iostream>

int main ()
{
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    std::cout << "Connecting to hello world server..." << std::endl;
    socket.connect ("tcp://localhost:5555");
    constexpr int num = 1000;

    //  Do 10 requests, waiting each time for a response
    for (int request_nbr = 0; request_nbr != 10; request_nbr++) {

        std::vector<float> values(num);
        for (size_t i = 0; i < num; i++) {
            values[i] = 0.434f;
        }
        char * data = new char[num];
        zmq::message_t request (num);

        memcpy (request.data (), data, num);
        std::cout << "Sending Hello " << request_nbr << "..." << std::endl;
        std::cout << "size:" << sizeof(data) << std::endl;
        socket.send (request);

        delete data;


        //  Get the reply.
        zmq::message_t reply;
        socket.recv (&reply);
        std::cout << "Received World " << request_nbr << std::endl;
    }
    return 0;
}
