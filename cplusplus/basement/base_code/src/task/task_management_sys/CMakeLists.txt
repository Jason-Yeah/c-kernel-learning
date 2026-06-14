cmake_minimum_required(VERSION 3.20)
project(UnmannedClusterDefense)

# 设置 C++ 标准为 C++11
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 将源文件列表存入变量
set(SOURCES
    main.cpp
    command.cpp
    datastore.cpp
    logger.cpp
    task.cpp
    task_manager.cpp
)

# 添加可执行文件
add_executable(main main.cpp ${SOURCES})

# 为目标添加编译定义，使 C++ 代码中可以使用 COMPILE_TIME 和 COMPILE_LOCATION 宏
target_compile_definitions(main PRIVATE 
)

# 设置可执行文件的输出名称，包含时间和地点信息
# 注意：文件名中不能包含空格和特殊字符，这里用下划线代替
set_target_properties(main PROPERTIES OUTPUT_NAME "main")