find_package(Threads REQUIRED)
find_package(PkgConfig)
find_package(Boost REQUIRED)
FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})


add_executable(asioserver asioserver.cpp)

target_link_libraries(asioserver ${Boost_LIBRARIES} Threads::Threads)


add_executable(asioclient asioclient.cpp)

target_link_libraries(asioclient ${Boost_LIBRARIES} Threads::Threads)