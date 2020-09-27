
add_executable(test1 server1.cpp)
add_executable(test2 server2.cpp)
add_executable(test3 server3.cpp)
add_executable(signal-test signal-test.cpp)
target_link_libraries(signal-test event_core)
