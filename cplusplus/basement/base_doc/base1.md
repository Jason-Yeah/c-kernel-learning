# 作用域

1. 全局
2. 局部
3. 块：实际上就是{....}中间的内容，比如场景有if, for, while或者单纯{}的代码块

```cpp
/* block */
void func()
{
    {
        int b_var = 5;
        // ... 这就是b_var当前的作用域
    }

    // int a = b_ var; 报错
}
```

## 类作用域

在类定义内声明的成员变量/函数所拥有的范围。

- 范围：成员变量在整个类的大括号内**都是可见的**
- 访问方式：在类外部访问时，取决于访问权限
- 特性：static静态成员虽然属于类作用域，但生命周期贯穿整个程序。

```cpp
class MyClass {
public:
    void display() {
        std::cout << val << std::endl; // 在类作用域内直接访问
    }
private:
    int val = 100; // 类作用域变量
};

int main() {
    MyClass obj;
    obj.display();
    // std::cout << obj.val << std::endl; // 错误！val 是私有的，类外不可直接访问
}
```

## 命名空间作用域

防止全局命名冲突而设计的逻辑分组。

- 范围：从声明点开始，直到该命名空间定义结束。它本质上是一种特殊的“全局作用域”。
- 访问方式：
    1. 使用作用域解析符`::`，如`std::cout`
    2. 使用`using`指令，如`using namespace`
- 特性：命名空间是开放的，可以在多个地方多次打开同一个命名空间来添加内容

```cpp
#include <iostream>

namespace CompanyA {
    int projectId = 555; // 属于 CompanyA 命名空间
}

namespace CompanyB {
    int projectId = 777; // 属于 CompanyB，不会与 A 冲突
}

int main() {
    // 使用作用域运算符访问
    std::cout << "Project A: " << CompanyA::projectId << std::endl;
    std::cout << "Project B: " << CompanyB::projectId << std::endl;
    return 0;
}
```

# 头文件

`.h`或`.hpp`文件是头文件，主要用于声明：

- Class
- 函数的原型prototypes
- 模板templates
- 宏定义#define
- 外部变量extern variables
- 内联函数inline functions

头文件通常包含预处理指令如`#ifndef #define #endif`，这些指令用于防止头文件被重复包含。头文件的存在让声明和定义分离。（`#pragma once`，写在文件最顶行也是一种方式）

示例：

## 编写头文件

```cpp
/* math_utils.h */
#include <iostream>

#ifndef MATH_UTILS
#define MATH_UTILS

int add(int a, int b);

#endif
```

注意：

- `< >`是编译器从系统标准库目录去寻找。
- `" "`是编译器先从当前目录下开始照，找不到再去标准目录库中。
- 定义宏一般是“项目名_路径名_文件名_H”做的

## pragma once

但这玩意不属于 C++ 正式标准（ISO C++），而是编译器厂商的一种约定

## extern解决重定位

问题：给头文件里定义变量了。

例如在point.h文件中定义了一个结构体

```cpp
// point.h
struct Point {
    int x;
    int y;
};
```

在另一个头文件里也用到了，也包含了上一个头文件

```cpp
// graphics.h
#include "point.h"

void drawLine(Point a, Point b);
```

在main.cpp中也包含了两个头文件

```cpp
// main.cpp
#include "point.h"    // 第一次包含 point.h
#include "graphics.h" // 包含 graphics.h，而它内部又包含了 point.h

int main() {
    Point p1 = {0, 0};
    return 0;
}
```

在编译器视角可能会是这样：

```cpp
// --- 来自第一次 #include "point.h" ---
struct Point {
    int x;
    int y;
};

// --- 来自 #include "graphics.h" ---
// 内部展开了它包含的 point.h
struct Point {  // <--- 报错：error: redefinition of 'struct Point'
    int x;
    int y;
};

void drawLine(Point a, Point b);

// --- 真正的 main 函数 ---
int main() {
    Point p1 = {0, 0};
    return 0;
}
```

解决方式：加入`#ifndef`他们仨或者`#pragma once`

```cpp
#pragma once  // 告诉编译器：这个文件无论被扫到多少次，只许处理一次

struct Point {
    int x;
    int y;
};
```

当前也有别的情况，比如一个.h文件有两个.cpp文件都包含了，最后链接的时候也会显示重定义

解决：`extern`关键字，表示声明的这个变量不是在此处定义的是在别的地方。示例：

```cpp
// config.h
// 或者 #pragma onec

#ifndef XXX_MATH_UTILES_H
#define XXX_MATH_UTILES_H
// int version = 1; // 报错！每个包含它的 .cpp 都会尝试创建一个 version 变量

extern int global_count; // 只声明不定义，不占内存

void increment();

#endif
```

在某个源文件中去定义

```cpp
#include "config.h"

// 真正的定义在这里，整个程序只准有一处
int global_count = 0; 

void increment() {
    global_count ++ ;
}
```


主程序中使用

```cpp
#include <iostream>
#include "math_utils.h"

int main() {
    increment();
    std::cout << "Global Count: " << global_count << std::endl; // 顺利访问
    return 0;
}
```