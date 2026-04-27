# 复合类型

- 数组
- 结构体
- 联合体
- 枚举
- 类
- 指针
- 引用

## 引用类型

### 定义

引用就是对另一个变量的别名。在C++中使用引用，可以直接访问和操作另一个变量的内存地址，不需要通过指针的解引用操作。

一旦引用被初始化，它就和原变量绑定在一起，对引用的操作实际就是对原变量的操作。

```text
类型 &引用名 = 原变量名；
```

示例：

```cpp
int a = 1;
int &var = a; // var是a的引用

var = 2; // 实际上a也变成了2
```

虽然在逻辑上引用是别名，但在底层实现中，编译器将引用实现为一个常量指针(`int* const p`)。

### 引用使用约束

1. 必须初始化，不能只声明；
2. 不可变，一旦初始化指向了一个变量，就不能再改为指向另一个变量
3. 无空引用，引用必须指向的是一个合法的内存。

### 使用

1. 函数参数，不需要去像指针那样频繁处理地址符号

    ```cpp
    void swap(int &a, int &b)
    {
        int t = a;
        a = b;
        b = t;
    }

    int main()
    {
        int a = 1, b = 2;
        swap(a, b);

        return 0;
    }
    ```

2. 提高效率，避免拷贝
    传递大型对象（如包含万个元素的`std::vector`或其他复杂类）时，传值会触发内存拷贝耗时，使用常量引用`const &`可避免内存拷贝也安全。

    ```cpp
    void func(const std::vector<int> &data)
    {
        // ...
    }
    ```

### 左值引用与右值引用

在C++11后，引用被细分为：

- 左值引用`T&`。指向有名字、有固定内存地址的对象（如普通变量，可修改）。
- 右值引用`T&&`。指向临时对象、即将销毁的对象（如表达式1 + 2）（不可修改的值如函数返回值，临时变量，字面量等）。这在移动语义（优化内存性能）中很重要。

```cpp
// l
int a = 10;
int& a_r = a;
// r
int&& b_r = 10;
```

右值引用作用：

#### 移动语义

在对象被销毁前窃取其资源（动态分配的内存、文件句柄等）而不是深度拷贝。

#### 完美转发

将参数原封不动传递给另一个函数，通过模板和`std::forward`函数实现。

## 指针类型

### 指针值

指针的值（地址）应属以下4种状态之一：

1. 指向一个对象
2. 指向紧邻对象所占空间的下一个位置`end()`
3. 空指针`nullptr`(nullptr可理解为字面量0)
4. 无效指针

### 万能指针`void *`

例如`void * vp = dp; // double *dp = &val;`这里vp是void *类型，可以复制但不能直接解引用，因为还不知道是什么类型

转类型`(double*)pv;`转类型并解引用`double t = *(double*)pv;`

这种类型指针主要用于做参数传递

### 指针引用

引用本身不是一个对象，因此不能定义指向引用的指针。但指针是对象，所以存在对指针的引用：

```cpp
int a = 10;
int *pi = nullptr;
int *&pr = pi;
pr = &a;
```

注意，对于

```cpp
int a[4] = {0, 1, 2, 3};

int *p = a;
p ++ ; // p = p + 1;
```

此时指针p存储的内存地址会+4，这是因为指针指向的是数组，它的移动步长是由指针所指向的数据类型的大小决定的。（这种操作仅限于指向数组元素的指针或迭代器）

# const

```cpp
const int a = 10;

int i1 = 1;
const int i2 = i1;
int i3 = i2;

// const int i4; 错误，必须定义不能只声明
```

const变量不能只声明，写了就必须要定义

### 编译器视角

在编译器将代码转译为汇编的过程中，编译器会在符号表中将该变量标记为“只读”（Read-only）

> 默认状态下，`const`对象仅在文件内有效

如果编译器确定一个 const 变量的值在编译时是已知的，它会进行“常量折叠”，编译器会像处理 #define 宏一样，直接在所有使用该变量的地方替换为它的实际数值（减少了运行时从内存中取值的次数）。

如果程序包含多个文件，则每个用了const对象的文件都必须得能访问到它的初始值才行。要做到这一点，就必须在每一个用到变量的文件中都有对它的定义.

为了支持这一用法，同时避免对同一变量的重复定义，默认情况下，const对象被设定为仅在文件内有效。当多个文件中出现了同名的const变量时，其实等同于在不同文件中分别定义了**独立的变量**。

```h
#ifndef CONST_VAL_H
#define CONST_VAL_H

const int val = 10;
// extern int val = 10; 这是其他情况防止重定义

#endif
```

验证：

```bash
Address of val: 0x560e7a625074
The address of val in main: 0x560e7a625098
```

在多个源文件中包含xxx.h头文件，发现可以编译通过，虽然多个源文件中包含了同名的const变量，但却是不同的变量，运行程序可以编译通过。

有时候我们不想定义不同的const变量，可以在.h头文件中用extern声明const变量减小内存开销。

```h
extern const int gval2;
```

验证可发现通过extern方式他们的地址是一样的

```bash
Address of val: 0x5f4055d7e08c
Address of gval2: 0x5f4055d7e088
The address of val in main: 0x5f4055d7e0d0
The address of gval2 in main: 0x5f4055d7e088
```

### 内存分配视角

1. 全局/静态const变量：编译器通常放在目标文件的.rodata section中
2. 局部const变量：编译器通常将其放在栈上

### const引用

```cpp
const int i = 1;
const int& r1 = i; // r1是常量引用，不能被修改，都是const int类型，可以引用

// int& r2 = i; 错误，类型不匹配，r2是int引用不是const int，从内存保护视角看，i是只读不可修改，如果r2能引用那就是int类型引用是可以修改，内存这个数据就不安全了，所以不能这样引用。

int a = 2;
const int& r2 = a; // 可以兼容因为a就是int可读可写，现在只是加上约束只读罢了，这里就是不能通过r2去修改a的值，但a本身可以自己去修改。
```

注：引用必须要和它绑定的对象是一个类型，但const下的有些特殊

```cpp
double dv = 1.87;
// int& r1 = dv; 错误类型不匹配
const int& r2 = dv; // 编译通过
```

这实际上是首先编译器用一个临时值做强制类型转换（double->int强行截断）`const int temp = dv;`然后再赋值回去`const int& r2 = temp;`

### 指针和const

`pointer to const`指向常量的指针，不能用于改变其所指向的对象

```cpp
const double PI = 3.14;
const double* ptr = &PI; // 或者double const* ptr2 = &PI;
```

### const指针

修饰的是指针，指针不能修改但指针指向的内容可以修改

```cpp
int err_num = 0;
int* const p = &err_num; // 表示const修饰的是p指针本身
// p ++ 报错，因为p本身不能变，比如p存的是0x7ffe3fb1fd04地址是，包括p = &out_num;也是错的
*p = 3; // 可以，因为p还是p没变只是指针指向的内容可以改，也就是0x7ffe3fb1fd04地址所在的内容随便动，0x7ffe3fb1fd04本身没动。
```

对于：

```cpp
const int* const p = &err_num; // 啥都不能变，左侧const锁了int*右侧const锁了p指针本身。
```

注意声明

```cpp
// int* const p1; 错误，const要求指针p1是不能动，但现在没初始化，也不知道指向哪里然后又不能变，编译器认为这种行为和Nt没区别
const int* p2; // 可以，const认为p2指向的地址解引用后的内容不能动，p2本身可以变，所以可以声明暂不定义。
```

### 顶层/底层const

指针本身是一个对象，它又可以指向另外一个对象。故指针本身是不是常量以及指针所指的是不是一个常量就是两个相互独立的问题。

- 顶层const（top-level const）表示指针本身是个常量【变量本身不能变】。`int* const p1 = &a`
- 底层const（low-level const）表示指针所指的对象是一个常量。`const int* p2 = &a`

示例：

```cpp
int i = 0;
//不能改变p1的值，这是一个顶层const
int * const pi = &i;
//不能改变ci的值，这是一个顶层const
const int ci  = 42;
//允许改变p2的值，这是一个底层const
const int *  p2 = &ci;
//靠右边的const是顶层const，靠左边的const是底层const
const int * const p3 = p2;
//用于声明引用的const都是底层const // 因为引用本身就不能改本身就是个隐式的顶层const
const int &r = ci; // 和int const &r = ci一样
```

底层const的限制却不能忽视。当执行对象的拷贝操作时，拷入和拷出的对象必须具有相同的底层const资格，或者两个对象的数据类型必须能够转换（从内存保护视角更好理解）

### constexpr和常量表达式

常量表达式（const expression）是指值不会改变并且在**编译阶段**就能得到计算结果的表达式。字面值属于常量表达式，用常量表达式初始化的const对象也是常量表达式

特殊情况：

```cpp
//sz不是常量表达式,运行时计算才得知
const int sz = GetSize();
```

尽管sz本身是一个常量，但它的具体值直到运行时才能获取到，所以也不是常量表达式

#### C++新标准

将变量声明为`constexpr`类型以便由编译器来验证变量的值是否是一个常量表达式。声明为constexpr的变量一定是一个常量，而且必须用常量表达式初始化

```cpp
//20是一个常量表达式
constexpr int mf = 20;
//mf+1是一个常量表达式
constexpr int limit = mf + 10;
//错误，GetSize()不是一个常量表达式，需要运行才能返回
//constexpr int sz = GetSize();
```

### 指针和constexpr

必须明确一点，在constexpr声明中如果定义了一个指针，限定符constexpr仅对指针有效，与指针所指的对象无关：

```cpp
//p是一个指向整形常量的指针
const int * p = nullptr;
//q是一个指向整数的常量指针，相当于**顶层指针**
constexpr int *q = nullptr;
//相当于
int* const q1 = nullptr;
```

一个constexpr指针的初始值必须是nullptr或者0，或者是存储于某个固定地址中的对象。
（函数体内（局部作用域里）定义的变量一般来说并非存放在固定地址中，因此constexpr指针不能指向这样的变量。）

### const和constexpr

const 告诉编译器“这个变量在运行时不能被修改”，而 constexpr 则是告诉编译器：“这个值必须在编译阶段就算出来！”

作用：

1. 空间换时间
2. 编译阶段检查bug

    ```cpp
    constexpr int get_max_connections() 
    {
        return 1000;
    }
    // 如果这个函数返回值不小心改成了负数，编译直接报错，根本生成不了可执行文件
    static_assert(get_max_connections() > 0, "连接数必须为正数！"); 
    ```

## inline

`inline`内联函数是 C++ 中用于向编译器提出优化建议的一个关键字，以“空间换时间”的策略，来提升程序的运行效率

inline 的作用就是建议编译器：在编译阶段，直接把函数体的代码“复制粘贴”到每一个调用该函数的地方，从而省去这些函数调用的开销。

```cpp
inline constexpr int get_size(){/*...*/};
```
