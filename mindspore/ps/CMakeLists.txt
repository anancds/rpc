set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../even-http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../)

include_directories(../../log)
include_directories(../../log/utils/)
#include_directories(../../../rpc)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "----------Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)

set(ABC_SRC
        util.cc
        worker.cc
        embedding_table_shard_metadata.cc
        )
set(ABC_HEADER
        util.h
        worker.h
        embedding_table_shard_metadata.h
        )

add_library(core3 ${ABC_SRC} ${ABC_HEADER})

#foreach (_target
#        util_test
#        )
add_executable(util_test11
        util_test.cpp
        ${ABC_SRC}
        ${ABC_HEADER}
        #            ${HTTP_SRC}
        #            ${HTTP_HEADER}
        #            ${PROTO_SRCS}
        )

target_link_libraries(util_test11
        event
        event_core
        Threads::Threads
        rpc-log
        protobuf::libprotobuf
        event_pthreads
        core1
        core2
        )
#endforeach ()