add_subdirectory(3rd_party/google-test)
set(CMAKE_CXX_STANDARD 17)
# enable CTest testing
enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../even-http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../log)

set(TEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/comm/tcp_client.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/comm/tcp_server.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/comm/tcp_message_handler.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/comm/comm_util.cc)

add_library(unittest ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/comm/http_message_handler.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/comm/http_server.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/ps/comm/comm_util.cc)
add_library(tcp ${TEST_SRC})

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
        )

add_executable(tcp_kv_server tcp_kv_server_test.cc ${PROTO_SRCS} ${PROTO_HDRS})


target_link_libraries(tcp_kv_server
        unittest
        tcp
        rpc-log
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        ${_PROTOBUF_LIBPROTOBUF}
        )

add_executable(tcp_client tcp_client_tests.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(tcp_client
        unittest
        tcp
        rpc-log
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        ${_PROTOBUF_LIBPROTOBUF}
        )

add_executable(comm_util common_util_test.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(comm_util
        unittest
        tcp
        rpc-log
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        ${_PROTOBUF_LIBPROTOBUF}
        )

add_executable(message_handler tcp_message_handler_test.cc ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(message_handler
        unittest
        tcp
        rpc-log
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        ${_PROTOBUF_LIBPROTOBUF}
        )

add_test(test_all unit_tests tcp_client tcp_server tcp_kv_server comm_util message_handler)