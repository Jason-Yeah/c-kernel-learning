# C/Kernel Learning

> 从 C/C++ 基础到操作系统、Linux 内核的逐层深入学习笔记与代码实践。

[![GitHub last commit](https://img.shields.io/github/last-commit/Jason-Yeah/c-kernel-learning)](https://github.com/Jason-Yeah/c-kernel-learning)
[![Language](https://img.shields.io/badge/language-C%20%7C%20C%2B%2B-blue)]()

## 📚 目录结构

```
ck_study/
├── cplusplus/          # C++ 学习
│   ├── basement/       # C++ 基础：类、继承、虚表、运算符重载、内存管理
│   └── effective_c++/  # 《Effective C++》读书笔记与代码示例
├── csapp/              # 《深入理解计算机系统》(CS:APP) 学习
│   ├── chapter9/       # 内存层次 & 缓冲区溢出攻击实验
│   ├── chapter10/      # 系统级 I/O
│   ├── chapter11/      # 网络编程
│   ├── chapter12/      # 并发编程（线程、互斥、条件变量）
│   └── csapp_lib/      # CS:APP 配套库 (csapp.c/csapp.h)
├── gdb/                # GDB 调试学习笔记与练习
│   └── src/            # 调试示例程序（含栈溢出挑战题）
├── lll/                # 底层链接与加载学习
│   ├── code/           # ELF、链接脚本、符号解析等实验
│   └── doc/            # 学习文档
├── makefile_study/     # Makefile 构建系统学习
│   └── docs/
└── operation_git.md    # Git 操作速查手册
```

---

## 🧩 项目详述

### C++ (cplusplus/)

| 子项目 | 内容 |
|--------|------|
| **basement** | 类与对象、继承、虚函数表、运算符重载、const 用法、智能指针与内存管理 |
| **effective_c++** | 《Effective C++》各条款学习笔记和配套 demo |

### 深入理解计算机系统 (csapp/)

按《Computer Systems: A Programmer's Perspective》原书章节组织实验代码：

- **ch9** — 虚拟内存 / malloc 实现 / 栈溢出攻击原理与 payload 构造
- **ch10** — Unix 系统级 I/O（文件描述符、重定向、stat）
- **ch11** — 套接字编程（echo server、地址转换）
- **ch12** — 并发编程（pthread、互斥锁、生产者-消费者、select 模型）

### GDB 调试 (gdb/)

GDB 使用技巧笔记，配合示例代码进行断点、内存查看、栈回溯等练习。

### 底层链接与加载 (lll/)

深入编译链接过程：ELF 文件结构、链接脚本、符号解析、重定位等。

### Makefile 构建 (makefile_study/)

Makefile 语法与实战，逐步构建中型项目的自动化编译。

---

## 🚀 快速开始

```bash
# 克隆仓库
git clone git@github.com:Jason-Yeah/c-kernel-learning.git
cd c-kernel-learning

# C++ 示例编译（以 basement 为例）
cd cplusplus/basement/base_code/src && g++ -o main main.cpp -I ../../inc

# CS:APP 示例编译
cd csapp/chapter10 && gcc -o stat_test stat_test.c

# 注意：栈溢出示例需关闭安全编译选项
cd csapp/chapter9 && gcc -fno-stack-protector -no-pie -o vuln vuln.c
```

---

## 🛠 环境

- **系统**: Ubuntu / Linux (WSL)
- **编译器**: GCC 11+ / G++
- **调试器**: GDB
- **构建**: GNU Make

---

## 📅 最近更新

*最后更新: 2026-05-24 22:15 CST*

- 完善 C++ 类与虚函数表相关 Demo
- 新增 C++ 运算符重载、this 指针练习
- CS:APP 并发编程线程示例整理
- 底层链接 ELF 文件结构分析
- 添加项目主页 README


