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
