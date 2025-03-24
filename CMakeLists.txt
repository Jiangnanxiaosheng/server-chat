cmake_minimum_required(VERSION 3.15)
project(server-chat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/common/config/config.ini
    ${CMAKE_CURRENT_BINARY_DIR}/config/config.ini
    COPYONLY
)


set(COMMON_SOURCES
    common/Singleton.hpp
    common/INIReader.hpp
    common/json.hpp
    common/common.hpp
)   

add_library(common INTERFACE ${COMMON_SOURCES})
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

# 添加协程库
add_subdirectory(vendor/coro)

# 添加各服务器子目录
add_subdirectory(GateServer)
add_subdirectory(StatusServer)
add_subdirectory(TcpChatServer)

############################
########## 使用GRPC
############################

# 查找 gRPC 和 Protobuf
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)


# 生成 proto 代码
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/common/proto")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
if(NOT GRPC_CPP_PLUGIN_EXECUTABLE)
  message(FATAL_ERROR "Cannot find grpc_cpp_plugin")
endif()

set(PROTO_FILE "${PROTO_DIR}/message.proto")

add_custom_command(
    OUTPUT "${GENERATED_DIR}/message.pb.cc" "${GENERATED_DIR}/message.pb.h"
    COMMAND protoc
    ARGS --cpp_out=${GENERATED_DIR} -I ${PROTO_DIR} ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating Protobuf files from ${PROTO_FILE}"
)

add_custom_command(
    OUTPUT "${GENERATED_DIR}/message.grpc.pb.cc" "${GENERATED_DIR}/message.grpc.pb.h"
    COMMAND protoc
    ARGS --grpc_out=${GENERATED_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE} -I ${PROTO_DIR} ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC files from ${PROTO_FILE}"
)

# 创建 proto 静态库
add_library(proto STATIC
    "${GENERATED_DIR}/message.pb.cc"
    "${GENERATED_DIR}/message.grpc.pb.cc"
)

target_include_directories(proto PUBLIC ${GENERATED_DIR})
target_link_libraries(proto PUBLIC gRPC::grpc++ gRPC::grpc++_reflection protobuf::libprotobuf)