# CLASS

## 作用

- 封装（Encapsulation）：将数据和操作数据的代码绑定在一起，保护数据不被外界直接访问。
- 抽象（Abstraction）：通过类定义抽象出具有共同特性的对象，提高代码的可重用性和可维护性。
- 继承（Inheritance）和多态（Polymorphism）：实现代码的复用与动态绑定。

## 成员变量与成员函数

成员变量

- 成员变量（Member Variables）：用于存储对象的状态信息。
- 命名约定：常用下划线结尾（例如 name_）表示成员变量，避免与局部变量混淆。

成员函数

- 成员函数（Member Functions）：定义对象的行为，可以访问和修改成员变量。
- 常成员函数（Const Member Functions）：保证函数不会修改对象的状态。（const对象只能调用常成员函数，绝不可调用非const函数）
  - 示例：
  
  ```cpp
  std::string get_name() const;

  std::string get_name() const {
        // name_ = "Tom"; // ❌ 如果在这里尝试修改 name_，编译器会直接报错！
        return name_;     // ✅ 只能读取
    }
  ```

## 访问控制

- public：公有成员，可以被所有代码访问。
- private：私有成员，仅能被类的成员函数和友元访问。
- protected：受保护成员，仅能被类的成员函数、友元和派生类访问。

## 构造函数&析构函数

构造函数

- 默认构造函数：没有参数的构造函数。
- 参数化构造函数：接受参数以初始化对象。
- 拷贝构造函数：用一个对象初始化另一个对象。
- 移动构造函数（C++11）：从临时对象“移动”资源。

析构函数

- 析构函数（Destructor）：在对象生命周期结束时调用，用于释放资源。

示例：

```cpp
Example() : data_(0)
{
    std::cout << "Default constructor called.\n";
}
Example(int data) : data_(data)
{
    std::cout << "Parameterized constructor called with data = " << data_ << ".\n";
}
// 拷贝构造函数
Example(const Example& other) : data_(other.data_)
{
    std::cout << "Copy constructor called.\n";
}
// 移动构造函数
Example(Example&& other) noexcept : data_(other.data_)
{
    other.data_ = 0;
    std::cout << "Move constructor called.\n";
}
// 析构函数
~Example() 
{
    std::cout << "Destructor called for data = " << data_ << ".\n";
}
```

说明：

- 写`Example e1;`时候就会调用默认的构造函数
- 写`Example e2(10);`就会调用参数化构造函数
- 写`Example e3 = e2;`会调用拷贝构造函数（深拷贝），参数是当前类的常量引用`const Example& other`
  - 当通过一个已存在的对象初始化新对象时，拷贝函数会在堆HEAP上重新申请一块全新的、大小相同的内存空间，然后逐步复制。（此时二者相互独立）
- 写`Example e4 = std::move(e2);`或者`Example e5 = Example(10);`的时候会调用移动拷贝函数
  - 接收一个右值引用，不申请新内存，把原对象指针里保存的“堆内存地址”赋值给新指针，然后把原对象指针置空
  - `noexcept`表示不会抛出异常；`std::move(e2)`：它的作用仅仅是把 e2 强制转换成一个右值引用。它相当于对编译器说：“我保证 e2 用完这次就不要了，你可以把它当成临时对象处理”，所以最后要把右值中的变量置空，不拥有任何堆内存。
    - 如果加入`e2`通过析构函数释放了（例如把0x1000标记为已回收），`e4`又去释放，就会出现“重复释放”，让程序崩溃。
- 析构函数（Destructor）：资源释放防止泄露，当对象生命周期结束（离开`}`或者被`delete`），由编译器自动调用，回收在构造函数中手动申请的堆内存
  - 执行 delete 操作，将指针指向的堆内存归还给操作系统。如果类中没有析构函数（或者析构函数里没写 delete），对象本身在栈上的内存会被回收，但它指针指向的堆内存就会变成没人能找到的“垃圾”，造成内存泄漏。

##  拷贝控制

拷贝函数：创建一个新对象，并复制现有对象的成员
`ClassName(const ClassName& other);`

拷贝赋值运算符：将一个已有对象的值赋值给另一个已有对象
`ClassName& operator = (const ClassName& other);`

示例：

```cpp
MyString& operator = (const MyString& other)
{
    std::cout << "Copy assignment operator called.\n";
    if (this == &other)
        return *this;
    
        delete[] data_;

        size_ = other.size_;
        data_ = new char[size_ + 1];
        std::strcpy(data_, other.data_);
        return *this;
}

~MyString()
{
    delete[] data_;
    std::cout << "Destructor called.\n";
}
```

解释：`if (this == &other) return *this;`检查a和b是不是一个东西，如果是`a=a`直接退出防止自己把自己删了。`delete[] data_;`假设原本a的`data_`有数据现在要通过b赋值，如果不把旧内存释放了a的指针会指向别的地方，原数据成孤儿内存泄漏，所以释放现有资源。而`delete[]`是因为`data_`是用`new char[n]`申请的，加上`[]`告诉系统这一整排房子全部拆除。

这里`this`是是一个指针，指向“当前正在调用这个函数的对象自己本身”

## 移动语义

允许资源的所有权从一个对象转移到另一个对象，避免不必要的拷贝，提高性能。

### 移动构造函数与移动赋值运算符

- 移动构造函数：`ClassName(ClassName&& other) noexcept;`
- 移动赋值运算符：`ClassName& operator=(ClassName&& other) noexcept;`

```cpp
class MoveExample {
public:
    // 省略构造函数
    // 移动构造函数
    MoveExample(MoveExample&& other) noexcept : size_(other.size_), data_(other.data_) {
        other.size_ = 0;
        other.data_ = nullptr;
        std::cout << "Move constructor called.\n";
    }

    // 移动赋值运算符
    MoveExample& operator=(MoveExample&& other) noexcept {
        std::cout << "Move assignment operator called.\n";
        if (this == &other)
            return *this;

        delete[] data_; // 防内存泄漏
        size_ = other.size_;
        data_ = other.data_;

        other.size_ = 0;
        other.data_ = nullptr;
        return *this;
    }
    // 析构函数省略
private:
    int size_;
    int* data_;
};

/*
  1. 自检查
  2. 自清空
  3. 赋值
  4. 参数置空
  5. 自返回
*/
```

## 类的友元

友元：可以访问类的私有和保护成员的非成员函数或另一个类。（许可证）

- 友元函数：单个函数可以被声明为友元。
- 友元类：整个类可以被声明为友元。

示例：

## 运算符重载

- 只能对已有运算符进行重载，不能创建新运算符。
- 至少有一个操作数必须是用户定义的类型。
- 不能改变运算符的优先级或结合性。

