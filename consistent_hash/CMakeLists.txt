
project(consistent_hash)

cmake_minimum_required(VERSION 3.1)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")

INCLUDE_DIRECTORIES(/usr/local/include)
LINK_DIRECTORIES(/usr/local/lib)

#ADD_EXECUTABLE(test test.cpp)
ADD_EXECUTABLE(example1 example.cpp)

ADD_EXECUTABLE(test111 test.cpp)
