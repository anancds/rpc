add_subdirectory(3rd_party/google-test)
set(CMAKE_CXX_STANDARD 17)
# enable CTest testing
enable_testing()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../even-http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../log)
add_library(unittest ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/http_message_handler.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/http_server.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/comm_util.cc)
add_library(tcp ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/tcp_client.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/tcp_server.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/tcp_message_handler.cc ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/comm_util.cc)
find_package(Threads REQUIRED)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")
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

add_executable(tcp_server tcp_server_tests.cpp)


target_link_libraries(tcp_server
        unittest
        tcp
        rpc-log
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        )

add_executable(tcp_kv_server tcp_kv_server_test.cc)


target_link_libraries(tcp_kv_server
        unittest
        tcp
        rpc-log
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        )

add_executable(tcp_client tcp_client_tests.cpp)

target_link_libraries(tcp_client
        unittest
        tcp
        rpc-log
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        )

#target_include_directories(unit_tests ${CMAKE_CURRENT_SOURCE_DIR}/../even-http)

add_test(test_all unit_tests tcp_client tcp_server tcp_kv_server)