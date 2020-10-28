set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(../log/)

add_executable(exampleold example_old.cpp http_message_handler.cc http_server.cc)
add_executable(exampletcp tcp_server_example.cpp tcp_server.cc comm_util.cc http_message_handler.cc tcp_message_handler.cc)
add_executable(exampletcpclient tcp_client_example.cpp comm_util.cc http_message_handler.cc tcp_message_handler.cc tcp_client.cc)

target_link_libraries(exampleold event event_core Threads::Threads rpc-log)
target_link_libraries(exampletcp event event_core Threads::Threads rpc-log)
target_link_libraries(exampletcpclient event event_core Threads::Threads rpc-log)
