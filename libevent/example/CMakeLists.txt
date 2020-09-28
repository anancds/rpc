
add_executable(test1 server1.cpp)
add_executable(test2 server2.cpp)
add_executable(test3 server3.cpp)
add_executable(signal-test signal-test.cpp)
target_link_libraries(signal-test event_core)

add_executable(learn server4.cpp)
target_link_libraries(learn event_core)

add_executable(tcp_echo_example tcp_echo_example.cpp)
target_link_libraries(tcp_echo_example event_core)
