set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -Wall -g -ggdb")

include_directories(../..)
include_directories(../../../log)
include_directories(
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/libevent/include"
        "${PROJECT_BINARY_DIR}"
)
link_directories(
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}/libevent/include"
)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)

file(GLOB_RECURSE PROTO_FILES "protos/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

find_package(nlohmann_json REQUIRED)

set(HTTP_SRC
        comm_util.cc
        node_manager_test.cpp
        tcp_client.cc
        tcp_message_handler.cc
        tcp_server.cc
        cluster_config.cc
        node.cc
        scheduler_node.cc
        server_node.cc
        worker_node.cc
        node_manager.cc
        http_server.cc
        http_message_handler.cc
        abstract_node.cc
        count_down_latch.cc
        thread_pool.cc
        http_client.cc
        worker_queue.cc
        ssl_util.cc
        file_configuration.cc
        )
set(HTTP_HEADER
        comm_util.h
        node.h
        node_manager_test.h
        tcp_client.h
        tcp_message_handler.h
        tcp_server.h
        cluster_config.h
        scheduler_node.h
        node_manager.h
        server_node.h
        worker_node.h
        abstract_node.h
        count_down_latch.h
        thread_pool.h
        message.h
        http_client.h
        worker_queue.h
        ssl_util.h
        configuration.h
        file_configuration.h
        )

add_library(core1 ${HTTP_SRC} ${HTTP_HEADER} ${PROTO_SRCS} ${PROTO_HDRS})
foreach (_target
        example_old scheduler_example
        client_example server_example client_test server_test protobuf_serialize_test fsm server_example1
        retry
        http_client_example
        function_test
        )
    add_executable(${_target} "${_target}.cpp"
            ${HTTP_SRC}
            ${HTTP_HEADER}
            ${PROTO_SRCS}
            ${PROTO_HDRS})
    target_link_libraries(${_target}
            event
            Threads::Threads
            rpc-log
            ${_PROTOBUF_LIBPROTOBUF}
            ssl
            crypto
            event_openssl
            nlohmann_json::nlohmann_json
            event_pthreads)
endforeach ()
