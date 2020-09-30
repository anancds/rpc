set(CMAKE_CXX_STANDARD  11)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

include_directories(${CMAKE_SOURCE_DIR})

add_executable(example example.cpp event_http_request.cpp event_http_server.cpp )

target_link_libraries(example event event_core Threads::Threads)