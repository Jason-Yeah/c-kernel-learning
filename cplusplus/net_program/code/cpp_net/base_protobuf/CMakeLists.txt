cmake_minimum_required(VERSION 3.20)
project(ProtobufDemo)

# 生成 compile_commands.json（clangd 靠这个识别包含路径）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找 protobuf（提供 PROTOBUF_GENERATE_CPP 宏）
include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

# 用 pkg-config 补全 absl 等依赖（新版 protobuf 需要）
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF_DEPS REQUIRED protobuf)

# 找 Boost（只需要头文件）
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# 生成的 .pb.h 在 build 目录下
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 编译 .proto 文件
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS chat.proto)

# 通用链接库（protobuf + absl 全依赖）
set(PROTOBUF_ALL_LIBS ${PROTOBUF_DEPS_LIBRARIES})

# hello_protobuf
add_executable(hello ${PROTO_SRCS} ${PROTO_HDRS} hello_protobuf.cpp)
target_link_libraries(hello ${PROTOBUF_ALL_LIBS})

# echo_server
add_executable(echo_server ${PROTO_SRCS} ${PROTO_HDRS} echo_server.cpp)
target_link_libraries(echo_server ${PROTOBUF_ALL_LIBS} pthread)

# echo_client
add_executable(echo_client ${PROTO_SRCS} ${PROTO_HDRS} echo_client.cpp)
target_link_libraries(echo_client ${PROTOBUF_ALL_LIBS} pthread)
