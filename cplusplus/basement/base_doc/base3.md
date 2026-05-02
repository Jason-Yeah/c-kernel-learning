# 类型

## 类型别名

```cpp
typedef double wages;
// p是double*也就是*wages的类型
typedef wages base, *p;
```

### C11新方式

新标准规定了一种新的方法，使用别名声明（alias declaration）来定义类型的别名：

```cpp
using int64_t = long long;
int64_t a = 10;
```

这种方法用关键字using作为别名声明的开始，其后紧跟别名和等号，其作用是把等号左侧的名字规定成等号右侧类型的别名。

### 指针、常量和类型别名

某个类型别名指代的是**复合类型或常量**，那么把它用到声明语句里就会产生意想不到的后果，例如：

```cpp
typedef char* pstring;
// 实际上是char* const str;
const pstring str = "fku";
// str ++ ; 报错
```

对于`const pstring str = "fku";`，首先`pstring`本质是指针，``const pstring str`意思是这个pstring类型的变量是常量。对于C++来说在编译这条语句时，认为const修饰的就是pstring（这一整个），如果是const char*，那修饰的是char不是char*那是内容，但现在pstring认为是一个整体，那就是指针本身了，常量指针。（如果是宏定义那和字面意思没问题）

但

```cpp
const pstring str = 0;
*str = 'm';
```

编译可以通过运行报错，编译是语法层面没问题，运行因为初始化str是0（NULL），在现在OS中，地址0所在的内存区域是受严格保护的，这么做`*str = 'm'`时，CPU尝试寻址到0x00..00位置然后写数据，发生段错误（Segmentation fault (core dumped)）

## auto类型

让编译器替我们去分析表达式所属的类型

## decltype类型指示符

“希望从表达式的类型推断出要定义的变量的类型，但是不想用该表达式的值初始化变量。”C++11新标准引入了第二种类型说明符`decltype`，它的作用是选择并返回操作数的数据类型。在此过程中，编译器分析表达式并得到它的类型，却不实际计算表达式的值

```cpp
decltype(f()) sum = x; //sum的类型就是函数f的返回值的类型
const int a = 10, &ra = a;
decltype(a) x = 0; // x是const int类型
decltype(ra) y = x; // y是const int&类型
int i = 10, *p = &i;
decltype(*p) pi = i; // pi是int&类型，也就是*p也是int&类型
decltype((i)) f = i; // i是int类型，(i)是表达式，计算当作是引用类型了
```

切记：decltype（（variable））（注意是双层括号）的结果永远是引用，而decltype（variable）结果只有当variable本身就是一个引用时才是引用。

工作中会利用auto和decltype配合使用，结合模板做类型推导返回动态类型（比如我们在并发编程系列课程中封装提交任务）

# Struct

使用场景

- 数据组织：将相关的数据组合，如学生信息、坐标点、日期等。
- 传递数据：在函数之间传递多个相关的数据项。
- 复杂数据处理：管理更复杂的数据结构，如链表、树、图等。

## 构造函数 

在 C++ 中，结构体可以像类一样拥有构造函数：

```cpp
struct Stu
{
    int id;
    std::string name;
    float grade;

    Stu(int s_id, std::string s_name, float s_grade) : 
        id(s_id), name(s_name) 
        {
            grade = s_grade;
        }
};
```

## 结构体和类

### 相似之处

- 都可以包含成员变量和成员函数。
- 都支持访问控制（public、protected、private）。
- 都可以使用继承和多态。

### 不同之处

#### 默认访问控制

- 结构体（struct）：默认成员访问权限为 public。
- 类（class）：默认成员访问权限为 private。

例子：

```cpp
struct StructExample {
    int x; // 默认 public
};

class ClassExample {
    int y; // 默认 private
};
```

用途习惯：

- 结构体（struct）：通常用于纯数据结构，主要存储数据，成员通常是公开的。
- 类（class）：用于包含数据和操作数据的函数，支持更加复杂的封装。

示例比较：

```cpp
struct Point {
    int x;
    int y;
};

class Rectangle {
private:
    int width;
    int height;

public:
    void setDimensions(int w, int h) {
        width = w;
        height = h;
    }

    int area() const {
        return width * height;
    }
};
```

```cpp
typedef struct {
    int id;
    std::string name;
    float grade;
} Student;

// 或者使用 `using`（C++11 及以上）
using Student2 = struct {
    int id;
    std::string name;
    float grade;
};
```

虽然结构体主要用于存储数据，但在 C++ 中，结构体也可以包含成员函数。这使得结构体更具面向对象的特性。

```cpp
#include <iostream>
#include <string>

struct Book {
    std::string title;
    std::string author;
    int pages;

    // 成员函数
    void printInfo() const {
        std::cout << "书名: " << title 
                  << ", 作者: " << author 
                  << ", 页数: " << pages << std::endl;
    }
};

int main() {
    Book myBook = {"C++ Primer", "Stanley B. Lippman", 976};
    myBook.printInfo();
    return 0;
}
```

# using命名空间

比如每次写`std::cout`太麻烦了，可以只使用这个

```cpp
#include <iostream>
#include <boost>

using std::cout;

int main()
{
    cout << "........" << std::endl;
    boost::cout << .....
}
```

位于头文件的代码一般来说不应该使用using声明。这是因为头文件的内容会拷贝到所有引用它的文件中去，如果头文件里有某个using声明，那么每个使用了该头文件的文件就都会有这个声明。对于某些程序来说，由于不经意间包含了一些名字，反而可能产生始料未及的名字冲突。
