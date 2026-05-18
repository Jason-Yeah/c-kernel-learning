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

## 运算符重载

- 只能对已有运算符进行重载，不能创建新运算符。
- 至少有一个操作数必须是用户定义的类型。
- 不能改变运算符的优先级或结合性。

## 继承多态

### 继承

```cpp
class Base {
    public:
        void baseFunction();
    protected:
        int protectedMember;
    private:
        int privateMember;
};

class Derived : public Base { // 公有继承
    public:
        void derivedFunction();
};
```

### 虚函数

`虚函数`允许派生类重新定义基类中的函数，以实现**多态性**。在运行时，根据对象的实际类型调用相应的函数版本（告诉编译器：“这个函数先别急着在编译时确定，等到程序真正运行时，看看实际是谁的对象，再决定调用哪个版本。”）。

程序在运行时知道该调用谁，是有一套经典的机制，叫虚函数表（vtable）和虚表指针（vptr）

- 虚函数表（vtable）：编译器会为每个包含虚函数的类，生成一张隐藏的“函数地址表”，里面按顺序存着这个类所有虚函数的实际地址。
- 每个对象在创建时，内部会多出一个隐藏的指针（vptr），它指向该对象所属类的虚函数表。

当你执行 ptr->speak() 时，程序会顺着 ptr 找到对象的 vptr，再通过 vptr 找到对应的 vtable，最后从表中查出 Dog::speak() 的真实地址去执行（`ptr->vptr->Class::func()`）。

#### 注意

1. **析构函数必须是虚函数**，通过父类指针去 delete 一个子类对象，父类的析构函数一定要加 virtual。
   - 如果不加：delete ptr; 时只会调用父类的析构函数，子类特有的资源（比如子类 new 出来的内存）得不到释放，导致内存泄漏。
   - 加了之后：程序会先调用子类的析构，再调用父类的析构。
2. 构造函数不能是虚函数：对象在构造的时候，虚表指针（vptr）还没初始化好，此时根本无法进行动态查找，所以 C++ 语法禁止构造函数为虚函数。
3. 触发多态条件：
   1. 存在继承关系。
   2. 父类中的函数被声明为 virtual。
   3. 通过**父类的指针或引用**去调用这个函数（如果是直接用对象调用 myDog.speak()，那只是普通的成员函数调用）。

## 纯虚类与抽象基类

纯虚函数 是在**基类中声明**但不提供实现的虚函数。包含至少一个**纯虚函数**的类称为 抽象基类（Abstract Base Class，ABC）。抽象基类不能被实例化，要求派生类必须实现所有纯虚函数才能被实例化。

```cpp
class Vehicle
{
    public:
        //只声明不实现，强制子类实现该函数
        virtual void start_engine() = 0;
        
        virtual ~Vehicle() {}
};

class Car : public Vehicle
{
    public:
        void start_engine() override
        {
            printf("fk\n");
        }
};

int main()
{
    Car car;
    car.start_engine();

    Vehicle *v1 = &car;
    v1->start_engine();

    return 0;
}
```

## 继承后的访问控制

继承时的`访问控制`决定了基类成员在派生类中的可访问性。继承方式主要有三种：public、protected 和 private。它们影响继承成员的访问级别

- 公有继承（public inheritance）：
  - 基类的 public 成员在派生类中保持 public。
  - 基类的 protected 成员在派生类中保持 protected。
  - 基类的 private 成员在派生类中不可访问。
- 保护继承（protected inheritance）：
  - 基类的 public 和 protected 成员在派生类中都变为 protected。
- 私有继承（private inheritance）：
  - 基类的 public 和 protected 成员在派生类中都变为 private。

销毁对象时，顺序与构造完全相反。析构函数不需要手动调用基类的析构函数，编译器会自动在派生类析构函数执行完毕后，自动调用基类的析构函数。**对象按“后创建的先销毁”的顺序出栈**。

## 容器与继承

C++ 容器（如 std::vector、std::list 等） 通常存储对象的副本，而非指向对象的指针。因此，当与继承结合使用时，可能导致 切片（Object Slicing） 问题，即仅存储基类部分，丢失派生类特有的信息。为了实现多态性，推荐使用指针或智能指针存储对象。

> **对象切片**

把一个派生类（子类）的对象，直接赋值或存入一个基类（父类）的变量或容器中时，编译器会只拷贝属于基类的那部分数据，而把派生类自己特有的数据全部“切掉”丢弃。

## memory库

提供了现代 C++ 内存管理的核心工具，它的出现是为了让开发者摆脱手动管理内存（使用`new`和`delete`）的繁琐与危险，通过一种名为**RAII（资源获取即初始化）**的机制

### 智能指针

`<memory>`库中最著名和最常用的组件就是智能指针，一共提供了三种智能指针：

- `std::unique_ptr`：独占所有；当一个资源在同一时间只应被一个所有者拥有时使用
- `std::shared_ptr`：共享所有；当有多个所有者需要共同拥有一块资源时使用。它内部维护一个引用计数，只有当最后一个指向该资源的 shared_ptr 也被销毁时，资源才会被释放。
- `std::weak_ptr`：弱引用/观察者；它不拥有资源，只是观察 shared_ptr 管理的资源。主要用于解决 shared_ptr 可能产生的循环引用问题，防止内存泄漏。

上述指针都是被销毁时，所占有的资源也会被释放。

`std::make_unique<CLAZZ>();`一个工厂函数，在堆内存创建对象，产生一个`unique_ptr<CLAZZ>`对象。

通过`vecpb.emplace_back();`进行隐式向上转型，设有`std::vector<std::unique_ptr<BASE>>`容器，`vecpb.emplace_back(std::make_unique<CLAZZ>());`接收了这个临时的`unique_ptr<CLAZZ>`，把它移动（move）进了容器，并自动变成了`unique_ptr<BASE>`类型。

### 场景示例

- std::unique_ptr：工厂函数：函数负责创建一个对象，并将其所有权“移交”给调用者
- std::shared_ptr：共享所有权的计数器。
- std::weak_pt：打破循环引用：主要作用是解决 shared_ptr 的一个致命问题：循环引用。
