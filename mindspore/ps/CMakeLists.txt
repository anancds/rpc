set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../even-http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../log)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../mindspore)

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

add_library(ps ${ABC_SRC} ${ABC_HEADER})
target_link_libraries(ps PUBLIC rpc-log)

#set(SRC
#        ../../log/utils/log_adapter.cc
#        ../../log/utils/logging_level.cc
#        ../../log/utils/ms_utils.cc
#        ../../log/utils/memcpy_s.c
#        ../../log/utils/memset_s.c
#        ../../log/utils/memmove_s.c
#        ../../log/utils/securecutil.c)
#set(HDR
#        ../../log/utils/log_adapter.h
#        ../../log/utils/overload.h
#        ../../log/utils/securec.h
#        ../../log/utils/securectype.h
#        ../../log/utils/slog.h
#        ../../log/utils/ms_utils.h
#        ../../log/utils/securecutil.h)
#add_library(log STATIC ${SRC} ${HDR})

foreach (_target
        util_test
        )
    add_executable(${_target}
            "${_target}.cpp"
            )

    target_link_libraries(${_target}
            event
            event_core
            Threads::Threads
            rpc-log
            protobuf::libprotobuf
            event_pthreads
            core1
            ps
            )
endforeach ()