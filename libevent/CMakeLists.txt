
add_executable(libeventtest test.cpp)

target_link_libraries(libeventtest event event_core)

add_executable(libeventtest1 test1.cpp)

target_link_libraries(libeventtest1 event event_core)

add_executable(client1 client1.cpp)

target_link_libraries(client1 event event_core)

add_executable(server1 server1.cpp)

target_link_libraries(server1 event event_core)

