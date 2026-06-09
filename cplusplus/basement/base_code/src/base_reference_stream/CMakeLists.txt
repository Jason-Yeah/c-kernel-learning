cmake_minimum_required(VERSION 3.20)
project(ref_demo
    VERSION 1.0.0
    DESCRIPTION "C++20 reference demo project"
    LANGUAGES CXX
)

# ── C++20 标准 ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── 编译选项 ────────────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W3 /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ── 输出到项目根目录 ────────────────────────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")

# ── 可执行文件 ──────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    ref_demo.cpp
    ref_demo.h
)
