cmake_minimum_required(VERSION 3.20)
project(LogicLayerDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 找 Boost/ASIO
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# 源文件
set(SOURCES
    main.cpp
    session.cpp
    session_manager.cpp
    logic_system.cpp
    login_mgr.cpp
    chat_mgr.cpp
    network_server.cpp
)

add_executable(server ${SOURCES})
target_link_libraries(server pthread)
