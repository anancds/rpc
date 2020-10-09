add_subdirectory(3rd_party/google-test)
set(CMAKE_CXX_STANDARD  11)
# enable CTest testing
enable_testing()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../even-http)
add_library(unittest ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/event_http_server.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../even-http/event_http_request.cpp)
find_package(Threads REQUIRED)

# Add a testing executable
add_executable(unit_tests unit_tests.cpp)

target_link_libraries(unit_tests
        unittest
        GTest::GTest
        GTest::Main
        GMock::GMock
        GMock::Main
        event event_core Threads::Threads
        )

#target_include_directories(unit_tests ${CMAKE_CURRENT_SOURCE_DIR}/../even-http)

add_test(test_all unit_tests)