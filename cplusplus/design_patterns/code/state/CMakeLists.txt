cmake_minimum_required(VERSION 3.16)
project(state_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 导出 compile_commands.json：clangd 靠它做索引，补全/跳转/报错都依赖它
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(state_demo
    main.cpp
    document.cpp
    draft_state.cpp
    review_state.cpp
    published_state.cpp
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(state_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
