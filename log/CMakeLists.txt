find_package(glog 0.4.0 REQUIRED)

# Detect operating systems
if (CMAKE_SYSTEM MATCHES "Linux*")
    set(TARGET_LINUX ON)
endif ()

if (CMAKE_SYSTEM MATCHES "Darwin*")
    set(TARGET_OSX ON)
endif ()

if (CMAKE_SYSTEM MATCHES "Windows*")
    set(TARGET_WIN ON)
endif ()


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)
set(SRC log_adapter.cc logging_level.cc ms_utils.cc memcpy_s.c memset_s.c)
set(HDR log_adapter.h overload.h securec.h securectype.h slog.h ms_utils.h)
add_library(rpc-log STATIC ${SRC} ${HDR})
target_link_libraries(rpc-log PRIVATE glog::glog)
# Export includes
target_include_directories(rpc-log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (TARGET_LINUX)
    target_compile_definitions(rpc-log PRIVATE -DTARGET_LINUX)
endif ()

if (TARGET_OSX)
    target_compile_definitions(rpc-log PRIVATE -DTARGET_OSX)
endif ()

if (TARGET_WIN)
    target_compile_definitions(rpc-log PRIVATE -DTARGET_WIN)
endif ()