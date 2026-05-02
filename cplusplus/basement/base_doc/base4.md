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
