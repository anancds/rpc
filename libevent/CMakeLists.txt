
find_package(Threads REQUIRED)

add_executable(libeventtest test.cpp)

target_link_libraries(libeventtest event event_core)

add_executable(libeventtest1 test1.cpp)

target_link_libraries(libeventtest1 event event_core)

add_executable(client1 client1.cpp)

target_link_libraries(client1 event event_core)

add_executable(server1 server1.cpp)

target_link_libraries(server1 event event_core)

add_executable(client3 client3.cpp)

target_link_libraries(client3 event event_core Threads::Threads)

add_executable(server3 server3.cpp)

target_link_libraries(server3 event_core Threads::Threads)

add_executable(client4 client4.cpp)

target_link_libraries(client4 event_core Threads::Threads)

add_executable(server4 server4.cpp)

target_link_libraries(server4 event_core Threads::Threads)

add_executable(uuid-test uuid.cpp)

target_link_libraries(uuid-test event_core Threads::Threads uuid)

add_subdirectory(example)
add_subdirectory(multithread)

