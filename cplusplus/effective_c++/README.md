# Effective C++ 学习笔记与实践代码

本项目是《Effective C++》（第三版）的中文学习记录，覆盖全书 55 个条款。

它不只是对原书结论的摘录，而是面向 C++ 基础学习者，重点补充：

- 每条建议试图解决什么问题；
- 错误写法为什么会出错；
- 编译器、对象模型、内存和操作系统层面的原因；
- 条款之间如何形成完整的设计逻辑；
- C++11～C++23 中应当怎样理解和应用原书原则；
- 可以独立编译、运行和调试的配套程序。

> 原书示例主要处于 C++98/03 时代。本项目不会机械照搬已经过时的接口，而是先解释原书背景，再补充现代 C++ 中的替代方案和仍然有效的设计原则。

## 项目内容

```text
effective_c++/
├── README.md                 # 项目入口（当前文件）
├── docs/                     # 按章节整理的详细学习笔记
│   ├── base_item_1to4.md
│   ├── base_item_5to12.md
│   ├── ...
│   ├── base_item_53to55.md
│   └── no_unique_address.md  # 补充专题
└── code/                     # 与各章笔记对应的可执行示例
    ├── item_1to4/
    ├── item_5to12/
    ├── ...
    └── item_53to55/
```

`docs` 中的主文档负责系统解释，`code` 中的程序负责验证现象。建议先读相关条款，再手动敲写或修改程序，而不是只看运行结果。

## 全书章节索引

| 章节 | 条款 | 主题 | 学习文档 | 配套代码 |
| --- | ---: | --- | --- | --- |
| 第一章 | 1～4 | 让自己习惯 C++ | [条款 1～4](docs/base_item_1to4.md) | [item_1to4](code/item_1to4/) |
| 第二章 | 5～12 | 构造、析构与赋值 | [条款 5～12](docs/base_item_5to12.md) | [item_5to12](code/item_5to12/) |
| 第三章 | 13～17 | 资源管理 | [条款 13～17](docs/base_item_13to17.md) | [item_13to17](code/item_13to17/) |
| 第四章 | 18～25 | 设计与声明 | [条款 18～25](docs/base_item_18to25.md) | [item_18to25](code/item_18to25/) |
| 第五章 | 26～31 | 实现 | [条款 26～31](docs/base_item_26to31.md) | [item_26to31](code/item_26to31/) |
| 第六章 | 32～40 | 继承与面向对象设计 | [条款 32～40](docs/base_item_32to40.md) | [item_32to40](code/item_32to40/) |
| 第七章 | 41～48 | 模板与泛型编程 | [条款 41～48](docs/base_item_41to48.md) | [item_41to48](code/item_41to48/) |
| 第八章 | 49～52 | 定制 `new` 和 `delete` | [条款 49～52](docs/base_item_49to52.md) | [item_49to52](code/item_49to52/) |
| 第九章 | 53～55 | 编译器诊断、标准库与 Boost | [条款 53～55](docs/base_item_53to55.md) | [item_53to55](code/item_53to55/) |

补充专题：[`[[no_unique_address]]`、EBO、对象布局和内存对齐](docs/no_unique_address.md)。

## 各章学习重点

### 条款 1～4：建立 C++ 基本认知

- C++ 是由多种编程模型组成的语言；
- `const`、`constexpr`、`enum`、`inline` 与宏的区别；
- 对象初始化和赋值不是一回事；
- non-local static 对象的跨翻译单元初始化顺序；
- 函数内 static 与现代线程安全初始化。

### 条款 5～12：对象的复制、赋值和生命周期

- 编译器会自动生成哪些特殊成员函数；
- `= delete`、移动语义和 Rule of Zero/Five；
- 多态基类为什么通常需要虚析构函数；
- 析构函数为什么不能让异常逃离；
- 构造和析构期间的虚函数调用；
- 自赋值、copy-and-swap 和完整复制对象状态。

### 条款 13～17：资源管理

- RAII 与异常栈展开；
- `unique_ptr`、`shared_ptr` 和所有权表达；
- 资源复制、转移与引用计数；
- 裸资源接口的边界；
- `new/delete` 的形式匹配；
- 对象创建过程中的异常安全。

### 条款 18～25：接口和类型设计

- 让错误用法难以表达；
- 按值传递与按引用传递；
- 返回局部对象、返回引用和对象生命周期；
- 数据封装与非成员非友元函数；
- 隐式转换、ADL、Pimpl 和 swap。

### 条款 26～31：实现细节

- 延迟变量定义；
- 尽量减少强制类型转换；
- 内部指针、引用和迭代器失效；
- 异常安全的基本、强和不抛保证；
- `inline` 的语言含义和优化含义；
- 头文件依赖、前置声明和 Pimpl。

### 条款 32～40：继承和面向对象设计

- public 继承表示 is-a；
- 名称隐藏与 `using`；
- 接口继承和实现继承；
- NVI、策略模式与虚函数；
- 非虚函数、默认参数和动态绑定；
- 组合、private 继承和空基类优化；
- 多重继承、菱形结构和虚继承。

### 条款 41～48：模板与泛型编程

- 隐式接口和编译期多态；
- dependent name、`typename` 和 `template` 消歧义；
- 模板化基类中的名称查找；
- 模板代码膨胀；
- 成员函数模板和兼容类型转换；
- hidden friend、ADL 和函数模板推导；
- traits、标签分派、concepts 和模板元编程。

### 条款 49～52：内存分配机制

- `new` 表达式、`operator new` 和构造函数的区别；
- `new-handler` 与 OOM 处理；
- allocator、内存池、arena 和 PMR；
- 大小、对齐、sized delete 和派生类分配；
- placement new、placement delete 和对象生命周期。

### 条款 53～55：工程工具和程序库

- 正确理解和治理编译器警告；
- 静态分析、sanitizer、测试和 code review 的职责；
- TR1 与现代标准库的历史关系；
- ranges、view、`span` 和非拥有对象的生命周期；
- Boost 的适用场景、依赖成本和接口隔离。

## 环境要求

建议使用：

- 支持 C++17 和 C++20 的编译器；
- GCC 11+、Clang 14+ 或功能相当的工具链；
- Linux/macOS，或提供相应 C++ 工具链的其他平台；
- GDB/LLDB，用于观察对象、调用栈和动态派发；
- 可选的 AddressSanitizer、UndefinedBehaviorSanitizer；
- Boost 为可选依赖，仅部分扩展示例使用。

当前较早章节的主体程序多以 C++17 编译；涉及 concepts、ranges、`[[no_unique_address]]`、`construct_at` 等内容时需要 C++20。每份章节文档末尾都有更准确的编译说明。

## 快速开始

进入项目：

```bash
cd /home/jason/study/ck_study/cplusplus/effective_c++
```

编译一个 C++17 示例：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g -O0 \
    code/item_13to17/item13_raii.cpp -o /tmp/item13
/tmp/item13
```

编译一个 C++20 示例：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -g -O0 \
    code/item_53to55/item54_standard_library.cpp -o /tmp/item54
/tmp/item54
```

使用 sanitizer 检查适合独立运行的程序：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -g -O0 \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    code/item_53to55/item54_standard_library.cpp -o /tmp/item54_san
/tmp/item54_san
```

`-O0` 方便初学阶段逐行调试，`-g` 生成调试信息，`-Wall -Wextra -Wpedantic` 打开常用诊断。它们不是生产环境构建参数的固定答案。

## 关于代码的编译方式

大多数以 `itemXX_...cpp` 命名的文件都包含独立的 `main`，应分别编译。不要把同一目录下所有 `.cpp` 无条件链接成一个程序，否则会出现多个 `main` 定义。

少数示例故意由多个翻译单元构成，例如：

```bash
cd code/item_1to4/static_initialization

g++ -std=c++17 -Wall -Wextra -Wpedantic \
    bad_config.cpp bad_logger.cpp bad_main.cpp -o /tmp/static_bad
```

条款 31 的分离编译示例：

```bash
cd code/item_26to31

g++ -std=c++17 -Wall -Wextra -Wpedantic \
    item31_widget.cpp item31_main.cpp -o /tmp/item31
```

目录中还可能存在 `test.cpp`、`cpp20.cpp`、`item44.cpp` 等学习过程中创建的辅助实验。章节文档表格列出的 `itemXX_描述.cpp` 是主要配套程序；辅助文件应按其自身内容单独判断编译方式。

## 建议学习方式

每个条款可以按以下顺序学习：

1. 阅读文档中的问题背景，不要先背最终规则；
2. 画出对象、指针、调用关系或生命周期；
3. 手动敲写最小示例并编译；
4. 修改一处关键代码，观察编译错误或输出变化；
5. 使用 GDB/LLDB 在构造、析构、虚函数或异常位置设置断点；
6. 用自己的话回答“为什么”，而不仅是“应该怎么写”；
7. 最后再把原则对应到现代标准库和真实项目设计。

建议优先进行的实验包括：

- 注释 `override`，再故意修改函数签名；
- 在 vector 扩容前后观察地址、引用和迭代器；
- 在抛出异常的位置单步观察栈展开和局部对象析构；
- 调换跨翻译单元链接顺序，观察静态初始化问题；
- 比较普通成员、空基类和 `[[no_unique_address]]` 的 `sizeof`；
- 给模板类型换成不满足隐式接口的类型，阅读完整诊断链。

## 现代 C++ 与原书的关系

原书的不少核心原则仍然有效，例如 RAII、类型安全、异常安全、接口设计和资源所有权。但某些具体建议需要结合现代语言重新判断：

| 原书时代常见内容 | 现代 C++ 中的理解 |
| --- | --- |
| `std::auto_ptr` | 已移除；使用 `std::unique_ptr` |
| 手写资源管理类 | 优先 Rule of Zero 和标准 RAII 类型 |
| `std::tr1` | 新代码使用已经标准化的 `std` 设施 |
| 裸 `new/delete` | 普通业务代码优先智能指针、容器和工厂函数 |
| SFINAE 技巧 | C++20 中许多接口可用 concepts 表达 |
| 递归模板元编程 | 优先 `constexpr`、`if constexpr` 和标准 traits |
| 空策略类 private 继承 | C++20 可评估组合加 `[[no_unique_address]]` |
| 自定义全局分配 | 先考虑 profiler、allocator、PMR 和成熟分配器 |

现代语法没有让原书原则失效，而是让许多原则能够表达得更直接、更安全。

## 阅读和修改代码时的注意事项

- “某平台常见实现”不等于 C++ 标准保证；文档会尽量区分语言规则与 ABI 实现。
- 地址、`sizeof`、虚表布局和链接顺序相关结果可能因编译器与平台而不同。
- 某些错误写法只保留在注释或宏控制分支中，避免默认运行未定义行为。
- 示例强调单一知识点，不应直接当作完整生产组件。
- 内存池、智能指针、容器和并发设施优先使用成熟标准库或经过验证的库。
- 若示例输出与文档不同，应先检查编译标准、优化级别、编译器版本和标准库实现。

## 当前完成情况

- [x] 条款 1～55 的章节笔记
- [x] 九章配套代码目录
- [x] 现代 C++11～C++23 相关补充
- [x] `[[no_unique_address]]` 专题说明
- [x] 主要示例的独立编译和运行验证

后续学习时可以继续在对应章节中增加自己的实验、GDB 记录和问题总结。对于已经理解的条款，最有效的巩固方式通常不是继续扩充笔记，而是在真实代码审查和设计中主动识别它。
