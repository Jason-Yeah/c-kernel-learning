# Task Management System

一个命令行任务管理系统，C++ 学习练手项目。

## 功能

| 命令 | 格式 | 说明 |
|------|------|------|
| add | `add <描述>, <优先级>, <截止日期>` | 添加任务（优先级 1=高 2=中 3=低） |
| delete | `delete <ID>` | 删除任务 |
| update | `update <ID>, <描述>, <优先级>, <截止日期>` | 更新任务 |
| list | `list` | 列出所有任务 |
| ls | `ls <0/1/2>` | 列任务（1=按优先级 2=按日期） |
| filter | `filter -k <关键词> -p <优先级> -d <起始> <截止>` | 多条件筛选 |
| exit | `exit` | 退出 |

### filter 示例

```
filter -k sb               # 描述含 sb 的任务
filter -p 1                # 高优先级任务
filter -d 2026-06-01 2026-07-01   # 日期范围内的任务
filter -k sb -p 1          # AND 组合条件
```

## 构建

```bash
mkdir build && cd build
cmake ..
make
./main
```

## 代码结构

```
├── main.cpp                 # 入口，命令分发
├── command.h / command.cpp  # 命令类（CRTP 模式）
├── command_registry.h / .cpp# 命令工厂（静态注册）
├── task_manager.h / .cpp    # 任务管理（增删改查 + 持久化）
├── task.h / task.cpp        # 任务数据结构
├── datastore.h / .cpp       # 数据存储
├── logger.h / logger.cpp    # 日志（单例 + 多线程安全）
└── CMakeLists.txt
```

## 涉及 C++ 特性

- **CRTP** — `command<Derived>` 静态多态
- **Type Erasure** — `wapper_command` 手写类型擦除
- **Singleton** — `logger` 函数局部静态变量
- **RAII** — 构造函数打开文件，析构关闭
- **Lambda + 算法** — `std::find_if`、`std::all_of` 函数式组合
- **静态注册** — `REGISTER_COMMAND` 宏 + IIFE 实现 main 前自动注册
- **异常安全** — try-catch 保护 `std::stoi`
