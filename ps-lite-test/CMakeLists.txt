message(STATUS "ps:${PROJECT_SOURCE_DIR}")
find_package(Threads REQUIRED)


include_directories(
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/ps-lite/include"
        "${PROJECT_BINARY_DIR}"
)
link_directories(
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}/ps-lite"
)

set(STA_DEPS pslite protobuf zmq)
add_executable(test_connection test_connection.cpp)
target_link_libraries(test_connection ${STA_DEPS} Threads::Threads)


add_executable(test_kv_app_benchmark test_kv_app_benchmark.cpp)
target_link_libraries(test_kv_app_benchmark ${STA_DEPS} Threads::Threads)