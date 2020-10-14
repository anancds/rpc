project(http-lib)

cmake_minimum_required(VERSION 3.1)

# Detect operating systems
if (CMAKE_SYSTEM MATCHES "Linux*")
    set (TARGET_LINUX ON)
endif()

if (CMAKE_SYSTEM MATCHES "Darwin*")
    set (TARGET_OSX ON)
endif()

if (CMAKE_SYSTEM MATCHES "Windows*")
    set (TARGET_WIN ON)
endif()


set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED 11)
set (SRC proto_client.cpp proto_server.cpp proto_utils.cpp)
set (HDR proto_client.h proto_server.h proto_utils.h)
add_library(msg_io STATIC ${SRC} ${HDR})

# Export includes
target_include_directories(msg_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Libraties are commented out for now. The reason is our projects use own library layout scheme
# (they are prebuilt in own repository).
 find_library(LIBEVENT libevent.a)
 find_library(LIBEVENT_PTHREADS libevent_pthreads.a)
# target_link_libraries(http_client_test pthread ${LIBEVENT} ${LIBEVENT_PTHREADS})

if (TARGET_LINUX)
    target_compile_definitions(msg_io PRIVATE -DTARGET_LINUX)
endif()

if (TARGET_OSX)
    target_compile_definitions(msg_io PRIVATE -DTARGET_OSX)
endif()

if (TARGET_WIN)
    target_compile_definitions(msg_io PRIVATE -DTARGET_WIN)
endif()

add_executable(msg_server msg_server.cpp)
target_link_libraries(msg_server msg_io event event_core event_pthreads)
add_executable(msg_client msg_client.cpp)
target_link_libraries(msg_client msg_io event event_core event_pthreads)
