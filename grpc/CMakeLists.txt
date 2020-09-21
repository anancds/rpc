set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Threads REQUIRED)

#find_package(protobuf REQUIRED)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf REQUIRED)
set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_PROTOBUF_PROTOC protobuf::protoc)
message(STATUS "Using protobuf ${protobuf_VERSION}")

find_package(gRPC CONFIG REQUIRED)
set(_GRPC_GRPCPP_REFLECTION gRPC::grpc++_reflection)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)

if (CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else ()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif ()

message(STATUS "Using gRPC cpp plugin ${_GRPC_CPP_PLUGIN_EXECUTABLE}")

#set(PROTOS
#        ${CMAKE_CURRENT_SOURCE_DIR}/../protos/helloworld.proto
#        )
#
#set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/core/proto_src)
#file(MAKE_DIRECTORY ${PROTO_SRC_DIR})

get_filename_component(hw_proto "${CMAKE_CURRENT_SOURCE_DIR}/../protos/helloworld.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)


# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

## Find gRPC CPP generator
#find_program(GRPC_CPP_PLUGIN NAMES grpc_cpp_plugin)
#mark_as_advanced(GRPC_CPP_PLUGIN)
#add_executable(gRPC::grpc_cpp_plugin IMPORTED)
#set_target_properties(gRPC::grpc_cpp_plugin PROPERTIES
#    IMPORTED_LOCATION ${GRPC_CPP_PLUGIN}
#)


# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/helloworld.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/helloworld.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/helloworld.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/helloworld.grpc.pb.h")
message(STATUS "bin: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "protoc: ${_PROTOBUF_PROTOC}")
message(STATUS "path: ${hw_proto_path}")
add_custom_command(
        OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="/usr/local/bin/grpc_cpp_plugin"
        "${hw_proto}"
        DEPENDS "${hw_proto}" protobuf::protoc gRPC::grpc_cpp_plugin)

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

foreach (_target
        greeter_client greeter_server)
    add_executable(${_target} "${CMAKE_CURRENT_LIST_DIR}/src/${_target}.cpp"
            ${PROTO_SRCS}
            ${GRPC_SRCS})
    target_link_libraries(${_target}
            ${_GRPC_GRPCPP_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})
endforeach ()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)