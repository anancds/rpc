set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

include_directories(../..)
include_directories(../../../log)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)

file(GLOB_RECURSE PROTO_FILES "protos/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set(HTTP_SRC
        comm_util.cc
        node_manager_test.cpp
        scheduler_manager.cpp
        server_manager.cpp
        tcp_client.cc
        tcp_message_handler.cc
        tcp_server.cc
        worker_manager.cpp
        cluster_config.cc
        node.cc
        scheduler_node.cc
        server_node.cc
        worker_node.cc
        node_manager.cc
        http_server.cc
        http_message_handler.cc
        )
set(HTTP_HEADER
        comm_util.h
        message.h node.h
        node_manager_test.h
        scheduler_manager.h
        server_manager.h
        tcp_client.h
        tcp_message_handler.h
        tcp_server.h
        worker_manager.h
        cluster_config.h
        scheduler_node.h
        node_manager.h
        server_node.h
        worker_node.h)

foreach (_target
        example_old scheduler_example
        client_example server_example client_test server_test protobuf_serialize_test)
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
            event_pthreads)
endforeach ()
