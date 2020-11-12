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
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)

set(STA_DEPS pslite protobuf zmq)
find_package(PkgConfig)
pkg_check_modules(ZMQ libzmq)

link_directories("${ZMQ_LIBRARY_DIRS}")

find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
        )

add_executable(test_connection test_connection.cpp)
target_link_libraries(test_connection ${STA_DEPS} Threads::Threads ${PROTOBUF_LIBRARY})
target_include_directories(test_connection PUBLIC ${PROTOBUF_INCLUDE_DIR})

add_executable(test_connection_server test_connection_server.cpp)
target_link_libraries(test_connection_server ${STA_DEPS} Threads::Threads ${PROTOBUF_LIBRARY} ${_PROTOBUF_LIBPROTOBUF} protobuf::libprotobuf-lite protoc grpc++)
target_include_directories(test_connection_server PUBLIC ${PROTOBUF_INCLUDE_DIR})

add_executable(test_connection_worker test_connection_worker.cpp)
target_link_libraries(test_connection_worker ${STA_DEPS} Threads::Threads ${PROTOBUF_LIBRARY} ${_PROTOBUF_LIBPROTOBUF} protobuf::libprotobuf-lite protoc grpc++)
target_include_directories(test_connection_worker PUBLIC ${PROTOBUF_INCLUDE_DIR})

add_executable(test_kv_app_benchmark test_kv_app_benchmark.cpp)
target_link_libraries(test_kv_app_benchmark ${STA_DEPS} Threads::Threads)

add_executable(test_kv_simple_app test_simple_app.cpp)
target_link_libraries(test_kv_simple_app ${STA_DEPS} Threads::Threads)