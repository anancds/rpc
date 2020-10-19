set(CMAKE_CXX_STANDARD  17)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

include_directories(${CMAKE_SOURCE_DIR})

add_executable(example example.cpp event_http_request.cpp event_http_server.cpp http_message_handler.cc http_server.cc)
add_executable(exampleold example_old.cpp event_http_request.cpp event_http_server.cpp http_message_handler.cc http_server.cc comm_util.cpp)
add_executable(exampletcp tcp_server.cc tcp_message_handler.cpp tcp_server_example.cpp comm_util.cpp)
add_executable(exampletcpclient tcp_client.cpp tcp_message_handler.cpp tcp_client_example.cpp comm_util.cpp)

target_link_libraries(example event event_core Threads::Threads rpc-log)

target_link_libraries(exampleold event event_core Threads::Threads rpc-log)
target_link_libraries(exampletcp event event_core Threads::Threads rpc-log)
target_link_libraries(exampletcpclient event event_core Threads::Threads rpc-log)
