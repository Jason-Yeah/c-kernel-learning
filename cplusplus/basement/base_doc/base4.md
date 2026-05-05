# string

std::string 是 C++ 标准库中的一个类，位于`<string>`头文件中。它封装了字符序列，并提供了丰富的成员函数用于操作字符串

存在多种初始化方式

```cpp
/* 源码：
namespace std _GLIBCXX_VISIBILITY(default){...
    {
        namespace pmr { using string = basic_string<char>; ...
    }...
}
*/
#include <string>

using std::string

int main()
{
    string s1;          // 默认初始化，空字符串
    string s2(s1);      // s2是s1副本
    string s3 = s1;     // 和上面等价
    string s4 = "jason";// 是人都能看懂
    string s5(5, 'c');  // 把s5初始化为由`4`个连续的字符'c'组成的串
    string s6(s4, 0, 4); //从0开始到第3个字符（左闭右开）
}
```

## 基本操作

```cpp
// 读string
std::string line;
std::cin >> line; // istream >> str | ostream << str
std::getline(std::cin, line); // getline(istream, str);
string str;
str.size();
str.empty();
str.append("fku"); // 在str后面追加

// 其他操作
std::string text = "xxxxxxx";
size_t pos = text.find(str); // size_t自适应unsigned int，32位是4B uint 64位是8B uint

if (pos != std::string::npos) 
{
    std::cout << "找到 '" << word << "' 在位置: " << pos << std::endl;
} 
else 
{
    std::cout << "'" << word << "' 未找到。" << std::endl;
}
```

## 其他操作

### 替换子字符串

```cpp
#include <iostream>
#include <string>

int main() 
{
    std::string text = "I like cats.";
    std::string from = "cats";
    std::string to = "dogs";

    size_t pos = text.find(from);
    if (pos != std::string::npos) 
    {
        // 位置，长度，替换什么
        text.replace(pos, from.length(), to);
        std::cout << "替换后: " << text << std::endl; // 输出: I like dogs.
    } else 
    {
        std::cout << "'" << from << "' 未找到。" << std::endl;
    }

    // 字符串切片
    std::string str = "Hello, World!";
    std::string sub = str.substr(7, 5); // 从位置7开始，长度5
    std::cout << sub << std::endl; // 输出: World

    // 字符串容量（容量 >= 长度）
    std::string str = "Hello";
    std::cout << "初始容量: " << str.capacity() << std::endl;

    str += ", World!";
    std::cout << "追加后的容量: " << str.capacity() << std::endl;
    return 0;
}
```

### 访问字符

对字符串中的字符操作，有如下方法, 切记需包含头文件`<cctype>`（其实就是`<ctype.h>`）

#### 检查越界

`.at(n)`函数

```cpp
std::string str = "ABCDE";
try 
{
    char c = str.at(10); // 超出范围，会抛出异常
} 
catch (const std::out_of_range& e) 
{
    std::cout << "异常捕获: " << e.what() << std::endl;
}
```

## 字符串流

`str::stringstream`是 C++ 标准库中`<sstream>`头文件提供的一个类，用于在内存中进行字符串的读写操作，类似于文件流。

示例：

```cpp
std::string data = "pi@192.168.10.102";
std::stringstream ss(data);
std::string res = ss.str();
std::cout << res << std::endl;
```

从字符串流中读取数据：

```cpp
std::string data = "pi 192.168.10.102";
std::stringstream ss(data);
std::string user;
std::string ip;

ss >> user >> ip;
std::cout << user << "@" << ip << std::endl;  
```

字符串与其他数据类型的转换：

```cpp
int num = 100;
double pi = 3.141;
// 使用std::to_string();
std::string str1 = std::to_string(num);
std::string str2 = std::to_string(pi);

std::cout << "str1: " << str1 << ", str2: " << str2 << std::endl;
```

## 正则表达式和字符串匹配

C++ 标准库提供了`<regex>`头文件，用于支持正则表达式。基本用法示例：

```cpp
#include <iostream>
#include <string>
#include <regex>

int main() {
    std::string text = "The quick brown fox jumps over the lazy dog.";
    std::regex pattern(R"(\b\w{5}\b)"); // 匹配所有5个字母的单词

    std::sregex_iterator it(text.begin(), text.end(), pattern);
    std::sregex_iterator end;

    std::cout << "5个字母的单词有:" << std::endl;
    while (it != end) {
        std::cout << (*it).str() << std::endl;
        ++it;
    }

    return 0;
}
```

# vector

## 它的迭代器

`std::vector<int>::iterator it = vec.begin()`对于std::vector来说，iterator通常是对它原生指针int*的一层轻量级封装把常用的运算符进行了重载用起来和指针很像。

## 修改元素

```cpp
vec[2] = 798;
print_vec(vec);
vec.at(2) = 857;
print_vec(vec);

try
{
    vec.at(100) = 111;
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}

for (std::vector<int>::iterator it = vec.begin(); it != vec.end(); it ++ )
    if (*it > 0) *it = - *it;
print_vec(vec);

// vec.assign(size_type __n, const value_type& __val); 清空重新赋值
/*
void
    assign(initializer_list<value_type> __l)
    {
this->_M_assign_aux(__l.begin(), __l.end(),
            random_access_iterator_tag());
    }
*/
vec.assign(4, 100);
print_vec(vec);

std::vector<int> vec2 = {1, 2, 3, 4, 5, 6};
vec.assign(vec2.begin() + 1, vec2.begin() + 4);
print_vec(vec);
```

### 彻底清楚容量

```cpp
printf("capacity: %ld\n", vec.capacity());
vec.clear();
printf("size: %ld\n", vec.size());
printf("capacity: %ld\n", vec.capacity());

// operation...
{
    std::vector<int> empty;
    empty.swap(vec);
}
printf("capacity: %ld\n", vec.capacity());
```

## 多维vector

```cpp
// 3表示个数，std::vector<int>(4, 0)表示每个元素是什么，这里表示每个元素都是一个Vector，其中每个vector都含有4个元素，每个元素都是0，总体就是3*4初始化为0的矩阵
std::vector<std::vector<int>> matrix(3, std::vector<int>(4, 0));
```

查找操作

```cpp
std::vector<std::string> nvec = {"111", "222", "333"};

std::string target = "222";
auto it = std::find(nvec.begin(), nvec.end(), target);

if(it != nvec.end()) {
    std::cout << target << " found at position " << std::distance(nvec.begin(), it) << std::endl;
}
else {
    std::cout << target << " not found." << std::endl;
}
```

首先std::find函数查找逻辑简单，就是从开始到结尾依次查$O(n)$，如果都没找到迭代器就到了end()的位置。

而`std::distance(_InputIterator __first, _InputIterator __last)`是计算两个迭代器之间的距离

## 性能优化

向量会动态地管理内存，自动调整其容量以适应新增或删除的元素。频繁的内存分配可能会影响性能。使用`reserve()`可以提前为向量分配足够的内存，减少内存重新分配的次数，提高性能。

```cpp
std::vector<int> vec3;
printf("capacity: %ld\n", vec3.capacity());
vec3.reserve(100);
printf("capacity: %ld\n", vec3.capacity());

for (int i = 0; i < 100; i ++ ) vec3.push_back(i);

printf("capacity: %ld\n", vec3.capacity());
/*
capacity: 0
capacity: 100
capacity: 100
*/
```

收缩容量

使用`shrink_to_fit()`可以请求收缩向量的容量以匹配其大小，释放多余的内存。

# ITERATOR

提供了一种方法，按顺序访问容器（如vector, list, map等）中的元素，而无需暴露容器的内部表示。迭代器就像是一个指针，但它比指针更加安全，因为它只能访问容器内的元素，并且它的类型与容器紧密相关。

和指针不一样的是，获取迭代器不是使用取地址符，有迭代器的类型同时拥有返回迭代器的成员。比如，这些类型都拥有名为begin和end的成员，其中begin成员负责返回指向第一个元素（或第一个字符）的迭代器。end成员则负责返回指向容器（或string对象）“尾元素的下一位置（one past the end）”的迭代器，也就是说，该迭代器指示的是容器的一个本不存在的“尾后（off the end）”元素。

## 泛型编程

C++程序员习惯性地使用`!=`，其原因和他们更愿意使用迭代器而非下标的原因一样：因为这种编程风格在标准库提供的所有容器上都有效。

只有string和vector等一些标准库类型有下标运算符，而并非全都如此。与之类似，所有标准库容器的迭代器都定义了==和！=，但是它们中的大多数都没有定义`<`运算符。因此，只要养成使用迭代器和！=的习惯，就不用太在意用的到底是哪种容器类型。

## 迭代器类型

const_iterator和指向常量的指针差不多，能读取但不能修改它所指的元素值。相反，iterator的对象可读可写。

```cpp
// 迭代器it, it能读写vector<int>的元素
std::vector<int>::iterator it;
// it2能读写string对象的字符
std::string::iterator it2;
// it3只能读元素，不能写元素
std::vector<int>::const_iterator it3;
// it4只能读字符,不能写字符
std::string::const_iterator it4;
```

如果vector对象或string对象是一个常量，只能使用const_iterator；如果vector对象或string对象不是常量，那么既能使用iterator也能使用const_iterator。

```cpp
for (cit = vec.cbegin(); cit != vec.cend(); cit ++ )
    std::cout << *cit << " ";
puts("");
```

begin和end返回的具体类型由对象是否是常量决定，如果对象是常量，begin和end返回const_iterator；如果对象不是常量，返回iterator

如果后期使用的是auto对一个非const容器，而此时又只想保证迭代器指向的数据只读，用cbegin()

```cpp
for (auto it = vec.cbegin(); it != vec.cend(); it ++ )
{
    ...
}
```

为了简化上述表达式，C++语言定义了箭头运算符（->）。箭头运算符把解引用和成员访问两个操作结合在一起，也就是说，it->mem和(*it).mem表达的意思相同。

```cpp
std::vector<std::string> text = {"aaa", "", "ccc"};
for (auto it = text.cbegin(); it != text.cend() && !it->empty(); it ++ )
    std::cout << *it << std::endl;
```

erase会返回删除元素的下一个元素的迭代器（vector容器存储了一系列数字，在循环中遍历每一个元素，并且删除其中的奇数，要求循环结束，vector元素为偶数，要求时间复杂度o(n)）

```cpp
void change_vec(std::vector<int> & vec)
{
    // 假的不是O(n)
    // for (auto it = vec.begin(); it != vec.end();)
    // {
    //     if (*it % 2) 
    //     {
    //         it = vec.erase(it);
    //         continue;
    //     }
    //     it ++ ;
    // }

    auto new_end = std::remove_if(vec.begin(), vec.end(), [](int x){
        return x % 2;
    });

    vec.erase(new_end, vec.end());
}
```

其中`remove_if`函数在`<algorithm>`库中，它实际上是重排，遍历容器**找到**所有不符合删除条件的元素（留下的是想要的）。把这些元素向前移动，覆盖掉要删除的元素，返回一个迭代器指向新序列的末尾。

```text
索引:    [0]  [1]  [2]  [3]
数值:     1    2    3    4
        ↑    
      write/read (都指向开头)

数值:    [2]   2    3    4  (第[0]位被覆盖成了2)
         ↑    ↑
       write read

数值:    [2]  [4]   3    4  (第[1]位被覆盖成了4)
              ↑         ↑
            write      read
```

# ARRAY

```cpp
//ptrs是含有10个整数指针的数组
int *ptrs[10];
//错误, 不存在引用的数组
//int& refs[10] = /*?*/;
//Parray指向一个含有10个整数的数组
int arr[10] ={0,1,2,3,4,5,6,7,8,9};
int (*Parray)[10] = &arr;
//arrRef 引用一个含有10个整数的数组
int (&arrRef)[10] = arr;
```

由内向外看数组，对于`int *ptrs[10];`意思是ptrs是大小为10个元素的数组，每个元素类型是int*也就是整形指针型的；`int (&arrRef)[10] = arr;`首先左边`&`表示arrRef是一个引用，再看右边10，说明引用的是一个大小为10的数组，最左边int表示数组里元素是整型变量，也就是说arrRef是数组arr的别名。它和原来的数组 arr 共享同一块内存。

`int (*Parr)[10] = arr;`指向数组的指针，首先`*Parry`表示这是个指针，右边10表示大小，最左边int表示说明数组里是整型变量，Parr是一个指针，里面是整个数组arr的地址，这里括号不能少，不然会变成第一种情况。

数组大小可以用sizeof确定

```cpp
int arr[10];
printf("%ld\n", sizeof(arr)); // 40 = 4 * 10
printf("%ld\n", sizeof(arr) / sizeof(int)); // 10
```

## C++11改进

为方便遍历数组，C++11提供了获取最后元素的下一个位置的指针，和指向首元素的指针

```cpp
/*
template<typename _Tp, size_t _Nm>
    [[__nodiscard__, __gnu__::__always_inline__]]
    inline _GLIBCXX14_CONSTEXPR _Tp*
    begin(_Tp (&__arr)[_Nm]) noexcept
    { return __arr; }
template<typename _Tp, size_t _Nm>
    [[__nodiscard__, __gnu__::__always_inline__]]
    inline _GLIBCXX14_CONSTEXPR _Tp*
    end(_Tp (&__arr)[_Nm]) noexcept
    { return __arr + _Nm; }
*/
int ia[] = {1, 2, 3, 4, 5};
int *iabeg = std::begin(ia);
int *iaend = std::end(ia);

for (auto it = iabeg; it != iaend; it ++ )
    std::cout << *it << " ";
std::cout << std::endl;

// calculating the element number of an array.
auto n = std::end(arr) - std::begin(arr);
std::cout << "n is " << n << std::endl;
```

## 使用array init vector

```cpp
auto n = std::end(ia) - std::begin(ia);
std::cout << "n is " << n << std::endl;
std::cout << "last element is " << *(std::begin(ia) + n - 1) << std::endl;

std::vector<int> vec(std::begin(ia), std::end(ia));
```

# 运算符

## 特殊运算符

`::*`用于声明一个指向类**成员**的指针

普通的指针（如 int*）直接指向内存中的某个具体地址，而`::*`声明的“成员指针”并不存储具体的内存地址，它存储的更像是一个“偏移量”。告诉你某个成员在类对象的内部“哪个位置”，但必须结合一个具体的对象，才能真正找到并访问那个成员。

用法：

```cpp
Type ClassName::* pointer_name = &ClassName::member_name;
```

示例

```cpp
class Person
{
    public:
        std::string name;
        int age;
        void sayhi()
        {
            std::cout << "Hi, I'm " << name << std::endl;
        }
};

int main()
{
    int Person::* prt_age = &Person::age;
    Person person1 = {"Jason", 23};
    Person person2 = {"Bob", 30};

    void (Person::*ptr_func)() = &Person::sayhi;

    std::cout << person1.age << std::endl;
    std::cout << person1.*prt_age << std::endl;
    std::cout << person2.*prt_age << std::endl;

    (person1.*ptr_func)();
}
```

对于`void (Person::*ptr_func)() = &Person::sayhi;`，在声明成员函数指针时，括号是为了把`*`和指针名ptr_func绑定在一起。如果不加，首先`()`优先级很高，编译器会认为，ptr_func是个普通成员函数，函数名就叫prt_fun，而返回类型是`void Person::*`

## 运算符重载

允许开发者为自定义类型（如类和结构体）定义或改变运算符的行为，使其表现得像内置类型一样

规则：

1. 几乎所有运算符都可以重载，但`:: ?: sizeof`等不可
2. 至少一个操作数必须是用户定义类型：即至少有一个操作数是类、结构体或联合体类型
3. 不修改原运算符优先级等等原性质

运算符可以作为成员函数或友元函数进行重载。对于成员函数：

```cpp
bool operator < (const Person& other) const{
    return this->age < other.age;
}
```

首先bool是类型，`operator <`是C++规定重载某个运算符的关键字，此时重载的是`<`；`const Person& other`参数使用常量引用，避免拷贝整个对象的开销，又保证不会在函数内部意外修改到被比较的对象other；末尾的const，加在函数参数列表后，表示改预算操作不会修改调用者自身（即this指针指向的对象）。

如果不加，当你在 std::set 等标准库容器中使用 Person 对象，或者比较的对象本身是 const 时，编译器就会直接报错。

## goto

`goto`允许无条件跳转到程序中指定的标签。虽然`goto`可简化代码，但不推荐使用，因为会使程序流程难以维护理解。示例：

```cpp
int n = 0;
std::cout << "n: ";
std::cin >> n;
if (n > 8) goto end;
std::cout << "fku" << std::endl;

end:
    std::cout << "n > 8, yeah." << std::endl;
```

# 异常处理

- try 块用于包含可能引发异常的代码（接锅侠）。
- throw 用于抛出异常（甩锅），可以在任何需要引发异常的位置使用，包括函数内部、嵌套调用中等。。
- catch 块用于捕获并处理异常（擦屁股）。

```cpp
double div(double a, double b)
{
    if (b == 0) throw std::invalid_argument("Denominator cannot be zero."); // 抛出异常
    return a / b;
}

try {
    double result = div(a, b);
    std::cout << "Result: " << result << std::endl;
} catch (std::invalid_argument &e) { // 捕获 std::invalid_argument 异常
    std::cerr << "Error: " << e.what() << std::endl;
}
```

其中`invalid_argument`是在`stdexcept`文件中定义的类，`e.what()`是标准异常类自带的一个函数，用来提取出里面的错误描述文字。

## rethrow异常

可以在 catch 块中使用 throw 语句重新抛出捕获的异常，以便其他部分处理。

```cpp
void func1() {
    throw std::runtime_error("Error in func1.");
}

// 函数，调用 func1 并重新抛出异常
void func2() {
    try {
        func1();
    } catch (...) { // 捕获所有异常
        std::cout << "func2() caught an exception and is rethrowing it." << std::endl;
        throw; // 重新抛出当前异常(func1的异常)
    }
}
```

# 函数重载

规则

- 函数名相同。
- 参数列表（类型、数量或顺序）不同。
- 返回类型不参与重载的区分

注意：

- 仅返回类型不同的重载是非法的。
- 默认参数可能会与重载产生冲突，使用时需谨慎。

## 默认参数

函数参数可以指定默认值，调用函数时可以省略这些参数，默认值将被使用。

规则

- 默认参数从右到左设置（←），不能部分设置。
- 函数声明和定义中默认参数只需在声明中指定。

```cpp
#include <iostream>

// 函数声明时指定默认参数
void displayInfo(std::string name, int age = 18, std::string city = "Unknown");

// 函数定义
void displayInfo(std::string name, int age, std::string city) {
    std::cout << "Name: " << name 
              << ", Age: " << age 
              << ", City: " << city << std::endl;
}

int main() {
    displayInfo("Bob", 25, "New York"); // 全部参数传递
    displayInfo("Charlie", 30);         // 省略city
    displayInfo("Diana");               // 省略age和city
    return 0;
}
```

# 内联函数

内联函数通过在函数前加inline关键字，建议编译器将函数代码嵌入到调用处，减少函数调用的开销。

适用于函数体积小、调用频繁的函数，如访问器（getter）和修改器（setter）等。

```cpp
inline double div(double a, double b)
{
    if (b == 0) throw std::invalid_argument("Denominator cannot be zero."); // 抛出异常
    return a / b;
}
try {
    double result = div(a, b);
    std::cout << "Result: " << result << std::endl;
} catch (std::invalid_argument &e) { // 捕获 std::invalid_argument 异常
    std::cerr << "Error: " << e.what() << std::endl;
}
```

好：

- 减少函数调用开销（栈操作）
- 提高性能
  
坏：

- 代码体积增大，可能影响缓存性能
- 编译器可能忽略内联请求，特别是对于复杂函数

# 尾递归：递归优化

尾递归是指递归调用在函数的最后一步，可以被编译器优化为循环，减少堆栈消耗（**函数在递归调用自身之后，不再执行任何其他的计算或操作**）。

```cpp
#include <iostream>

// 辅助函数，用于尾递归
long long factorialHelper(int n, long long accumulator) {
    if(n == 0)
        return accumulator;
    return factorialHelper(n - 1, n * accumulator);
}

// 尾递归函数
long long factorial(int n) {
    return factorialHelper(n, 1);
}

int main() {
    int number = 5;
    std::cout << number << "! = " << factorial(number) << std::endl;
    return 0;
}
```

factorialHelper函数的递归调用是函数的最后一步，编译器可以将其优化为迭代，减少堆栈消耗。

> 普通递归的隐患：每次递归都会在内存的“调用栈”上压入一个新的栈帧。如果递归层数太深（比如计算一百万的阶乘），栈空间会被耗尽，导致栈溢出（Stack Overflow），程序直接崩溃。
尾递归的优化：因为尾递归在调用下一步时，当前栈帧已经没用了，编译器会聪明地复用当前的栈帧，而不是开辟新的。这就相当于把递归转换成了一个高效的循环（while 或 for），无论递归多少次，占用的内存空间都是恒定的，彻底避免了栈溢出。

# Lambda表达式

Lambda表达式是C++11引入的匿名函数，便于在需要函数对象的地方快速定义和使用函数。它允许定义内联的、小型的可调用对象，无需单独定义函数。

语法：

```cpp
[ capture_list ] ( parameter_list ) -> return_type {
    // function body
}
```

其中`[ capture_list] `是让函数能够使用外部作用域的变量

- `[]` 不捕获任何外部变量，完全独立
- `[=]` 以拷贝（传值）方式捕获外部所有局部变量
- `[&]` 以引用方式捕获
- `[a, &b]` 混合捕获，拷贝外部变量a，引用外部变量b

示例

其中`std::for_each`是C++标准库 <algorithm> 头文件中提供的一个遍历算法，一个专门用来遍历容器（比如 std::vector、std::list 等）算法，只需要告诉它起点、终点，以及“对每个元素要做什么”，它就会自动帮你把容器里的每个元素都处理一遍。

基本用法：接受三个参数：it_start, it_end, op(Lambda)。

```cpp
for_each(_InputIterator __first, _InputIterator __last, _Function __f)
{
    // concept requirements
    __glibcxx_function_requires(_InputIteratorConcept<_InputIterator>)
    __glibcxx_requires_valid_range(__first, __last);
    for (; __first != __last; ++__first)
        __f(*__first);
    return __f; // N.B. [alg.foreach] says std::move(f) but it's redundant.
}
```
