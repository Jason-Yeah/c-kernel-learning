cmake_minimum_required(VERSION 3.20)
project(template_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(logger
    # template.cpp
    # template.h
    # sfinae_demo.h
    # sfinae_demo.cpp
    logger.cpp
    logger.h
)

# 配置阶段就会输出，帮你确认 C++20 是否真的生效了
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# 可选：如果 <concepts> 或 <format> 找不到，在这里快速定位
target_compile_options(logger PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
)
