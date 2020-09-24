list(APPEND CMAKE_MODULE_PATH "/home/cds/code/seastar/cmake")
find_package (Seastar REQUIRED)
include_directories(include)
add_executable(tcp_sctp_server_demo tcp_sctp_server_demo.cpp)

target_link_libraries(tcp_sctp_server_demo Seastar::seastar)

add_executable(tcp_sctp_client_demo tcp_sctp_client_demo.cpp)

target_link_libraries(tcp_sctp_client_demo Seastar::seastar)