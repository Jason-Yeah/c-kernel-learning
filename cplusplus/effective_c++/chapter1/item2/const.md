# const

## 顶层const与底层const

```cpp
const int* const p = new int(10);
```

左边是底层（修饰p指向的），右边是顶层（修饰p不可改变）

比如`const int a = 10;`这个就是顶层，const修饰的**变量**不可改变就是顶层const，这里a就是

比如`const int &ra = 10;`这个就是底层

当执行对象copy操作的时候，顶层const不受啥影响（大部分忽略），底层const要保持一致。

为什么？

- 顶层const保护我，对象本身是常量，那你拷贝的话只拷贝数值又跟你本身是啥没关系也不懂本身，自然大部分情况忽略了。
- 底层const锁住的是它，是指针所指的对象是常量，比如`const int* p1`表示不可以通过指针去改整数，这是防止权限越权。

```cpp
const int* const p = new int(10);
int *p1 = p; // 错误
```

这里第一行顶层const忽略，底层const一致，就是`const int* p`

## 内存分配维度

1. 全局/静态const：当定义`static const int x = 2;`或全局`const int g = 10;`时候，编译器一般会把这些放在全局.rodata段中。（有时候会优化，因为编译器知道这玩意可能永远不变，就不给分配内存了，直接把2嵌入机器指令中）。
2. 局部const：当定义如`void func() {const int x = 5; }`时，它在栈上，他和普通变量在一块，从硬件层面看这块是可读可写的，这里不可修改全靠编译器守着。

## 邪修理解

C++语法声明从右向左读比较顺，把`*`读为指向xxx的指针

- int* p: p是一个指针指向int。
- const int * p: p是一个指针指向const int，指向的东西是只读的。
- int* const p: p是一个只读的指针指向int，指针本身是只读的，指向的东西没说。
- const int * const p: p是一个只读的指针，指向int的内容也是只读的。
