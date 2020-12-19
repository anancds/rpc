add_subdirectory(3rd_party/google-test)
set(CMAKE_CXX_STANDARD 17)
# enable CTest testing
enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../even-http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../mindspore)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../log)

#set(TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/tcp_client.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/tcp_server.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/tcp_message_handler.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/comm_util.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/cluster_config.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/node.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/node_manager.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/scheduler_node.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/server_node.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/worker_node.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/node_manager_test.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/abstract_node.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/embedding_table_shard_metadata.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/worker.cc
#        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/util.cc
#        )

add_library(unittest ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/http_message_handler.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/http_server.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/comm_util.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/core/cluster_config.cc)
#add_library(tcp ${TEST_SRC})

find_package(Threads REQUIRED)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)

file(GLOB_RECURSE PROTO_FILES "protos/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})


find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

# Add a testing executable
add_executable(unit_tests unit_tests.cpp)

target_link_libraries(unit_tests
        unittest
        rpc-log
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        event_pthreads
        )


foreach (_target
        tcp_kv_server_test tcp_client_tests
        common_util_test tcp_message_handler_test cluster_connection_test cluster_connection_failed_test
        embedding_table_shard_metadata_test
        cluster_config_test
        worker_test
        count_down_latch_test
        )
    add_executable(${_target} "${_target}.cc"
#            ${PROTO_SRCS}
#            ${PROTO_HDRS}
            )
    target_link_libraries(${_target}
            unittest
            core1
            ps
            rpc-log
            GTest::GTest
            GTest::Main
            GMock::GMock
            GMock::Main
            event event_core Threads::Threads
            ${_PROTOBUF_LIBPROTOBUF}
            event_pthreads)
endforeach ()

add_test(test_all unit_tests
        tcp_kv_server_test
        tcp_client_tests
        common_util_test
        tcp_message_handler_test
        cluster_connection_test
        cluster_connection_failed_test
        embedding_table_shard_test
        worker_test
        )