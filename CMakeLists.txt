cmake_minimum_required(VERSION 3.17)
project(rpc)

set(CMAKE_CXX_STANDARD 11)
SET(GCC_COVERAGE_LINK_FLAGS    "-lboost_ program_options")

find_package(PkgConfig)
find_package(nlohmann_json REQUIRED
        CONFIG
        PATHS "/usr/lib/cmake")
find_package(Boost REQUIRED)
FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

pkg_check_modules(ZMQ libzmq)

link_directories("${ZMQ_LIBRARY_DIRS}")

find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
        )

add_executable(server hwserver.cpp zhelpers.hpp BenchmarkLogger.hpp)

target_link_libraries(server "${ZMQ_LIBRARIES}" ${Boost_LIBRARIES})

add_executable(client hwclient.cpp)

target_link_libraries(client "${ZMQ_LIBRARIES}" ${Boost_LIBRARIES})
