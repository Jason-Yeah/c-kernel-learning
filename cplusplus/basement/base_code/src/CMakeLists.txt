cmake_minimum_required(VERSION 3.10)

project(example_pro VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# include
include_directories(../inc)

set(SOURCE_FILES math_main.cpp math_utils.cpp)

add_executable(math_main ${SOURCE_FILES})

# cd build && cmake ../ 
# make