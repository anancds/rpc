
find_package(Threads REQUIRED)

add_executable(multithread echoserver_threaded.c workqueue.c)
target_link_libraries(multithread event_core Threads::Threads)