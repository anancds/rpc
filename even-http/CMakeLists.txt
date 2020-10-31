set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(../log/)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)

file(GLOB_RECURSE PROTO_FILES "protos/comm.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set(HTTP_SRC comm_util.cc network_manager.cpp node.cpp node_manager.cpp scheduler_manager.cpp server_manager.cpp tcp_client.cc tcp_message_handler.cc tcp_server.cc worker_manager.cpp)
set(HTTP_HEADER comm_util.h message.h network_manager.h node.h node_manager.h scheduler_manager.h server_manager.h tcp_client.h tcp_message_handler.h tcp_server.h worker_manager.h)

add_executable(exampleold example_old.cpp http_message_handler.cc http_server.cc)
add_executable(exampletcp tcp_server_example.cpp ${HTTP_SRC} ${HTTP_HEADER} ${PROTO_SRCS} ${PROTO_HDRS})
add_executable(exampletcpclient tcp_client_example.cpp ${HTTP_SRC} ${HTTP_HEADER} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(exampleold event event_core Threads::Threads rpc-log)
target_link_libraries(exampletcp event event_core Threads::Threads rpc-log ${_PROTOBUF_LIBPROTOBUF})
target_link_libraries(exampletcpclient event event_core Threads::Threads rpc-log ${_PROTOBUF_LIBPROTOBUF})
