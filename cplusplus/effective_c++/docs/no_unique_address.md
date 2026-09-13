# `[[no_unique_address]]` 详解

`[[no_unique_address]]` 是 C++20 引入的标准属性，主要用于告诉编译器：

> 这个非静态数据成员不一定需要占据一块完全独立、地址唯一的存储空间。

它最常见的用途是压缩空策略类、空删除器和空函数对象所占的空间，也允许编译器复用非空成员末尾的填充字节。不过，它只给编译器提供优化许可，并不保证 `sizeof` 一定变小。

## 1. 先理解：空类为什么 `sizeof` 通常是 1

```cpp
struct EmptyPolicy {};

static_assert(sizeof(EmptyPolicy) >= 1);
```

`EmptyPolicy` 没有数据成员，但一个独立的 `EmptyPolicy` 对象仍需要能够拥有地址。考虑数组：

```cpp
EmptyPolicy objects[2];
```

`&objects[0]` 和 `&objects[1]` 必须能够表示两个不同的数组元素。如果空对象大小为 0，数组中的两个元素就无法按照正常对象模型排列。因此，完整空类对象的大小不能是 0；常见实现令其大小为 1 字节。

注意，这不表示空类真的保存了 1 字节业务数据。这个字节主要是为了对象布局和地址区分。

## 2. 普通空成员为什么可能增大整个类

```cpp
struct EmptyPolicy {};

struct WithMember {
    EmptyPolicy policy;
    int value{};
};
```

在常见的 `int` 按 4 字节对齐的平台上，可能得到：

```text
policy       1 字节
padding      3 字节
value        4 字节
合计         8 字节
```

其中 padding 是编译器为了让 `value` 位于合适的对齐地址而插入的填充。于是，一个不保存状态的 policy 可能间接让对象从 4 字节变成 8 字节。

这里只能说“可能”，不能把 8 当作语言标准保证的固定结果。对象大小、对齐和填充受类型、ABI、编译器与目标平台影响。

## 3. EBO：为什么空基类常常不占额外空间

在 C++20 之前，常见做法是让类继承空策略：

```cpp
struct EmptyPolicy {};

struct WithBase : private EmptyPolicy {
    int value{};
};
```

编译器通常可以把空基类子对象放到与其他存储相同的起始位置，使它不增加 `WithBase` 的大小。这称为：

- EBO：Empty Base Optimization；
- EBCO：Empty Base Class Optimization；
- 中文通常称“空基类优化”。

因此常见结果是：

```text
sizeof(WithMember) == 8
sizeof(WithBase)   == 4
```

EBO 是“空类型作为基类”的布局能力。可是为了节约空间而继承会混入继承语义，也可能使类层次更复杂。如果 policy 在概念上是对象拥有的一个实现组件，那么组合通常更容易理解。

## 4. `[[no_unique_address]]` 做了什么

C++20 可以继续使用成员组合，同时把该成员声明为“可能重叠的子对象”：

```cpp
struct EmptyPolicy {};

struct WithAttribute {
    [[no_unique_address]] EmptyPolicy policy;
    int value{};
};
```

常见实现可让 `policy` 与 `value` 使用相同的起始地址，或以其他合法方式复用存储。因此，常见结果是：

```text
sizeof(WithAttribute) == sizeof(int)
```

这里并不是 `policy` 被删除了：

- `policy` 仍然是一个真正的成员；
- 仍可写 `object.policy`，也会遵守构造和析构规则；
- 如果它以后变成非空类，其状态仍必须得到保存；
- 只是语言允许它不必独占一段存储。

可以把三种布局的设计意图概括为：

```text
普通成员
  EmptyPolicy 独占位置 → 可能产生额外填充

private 空基类
  依靠 EBO 省空间 → 使用了继承

[[no_unique_address]] 成员
  保持组合关系 → 允许编译器复用存储
```

## 5. 为什么属性叫 `no_unique_address`

普通成员通常对应对象内部自己的地址位置。加上该属性后，成员不再要求拥有一块唯一的地址区域。例如在某个平台上，下面两个比较可能为真：

```cpp
WithAttribute object;

std::cout << static_cast<void*>(&object) << '\n';
std::cout << static_cast<void*>(&object.policy) << '\n';
std::cout << static_cast<void*>(&object.value) << '\n';
```

你可能观察到 `object.policy` 和 `object.value` 的地址数值相同，但它们仍是类型和语义不同的子对象。地址数值相同不等于“它们是同一个 C++ 对象”。

因此，不要根据地址相等就用 `reinterpret_cast` 随意跨类型读写；对象的类型、生命周期、别名规则仍然有效。

## 6. 为什么说它“允许”而不是“强制”优化

这正是主文档最后一句话的含义。

`[[no_unique_address]]` 改变的是布局许可，不是对 `sizeof` 的硬性承诺。下面代码不适合写成可移植断言：

```cpp
// 不推荐：标准没有保证在所有实现上都成立。
static_assert(sizeof(WithAttribute) == sizeof(int));
```

编译器可能因为以下因素没有表现出你预期的缩减：

- 当前成员并非空类型；
- 同类型子对象仍需满足相应的地址区分约束；
- 对齐要求阻止某种重叠方式；
- ABI 必须保持既定类布局；
- 编译器对相关布局优化的实现程度不同；
- 调试、目标架构和编译器版本不同。

所以测试程序应输出 `sizeof` 供观察，而不应把某个本机结果当成 C++ 标准规定。

## 7. 不只对空类型有意义：尾部填充复用

某些非空类型因为对齐要求，在末尾也可能存在填充字节：

```cpp
struct Padded {
    int number;
    char flag;
    // 某些平台会在末尾加入 padding。
};

struct Holder {
    [[no_unique_address]] Padded padded;
    char marker;
};
```

该属性允许实现考虑复用 `padded` 的尾部填充来保存后续成员。不过，这类效果比空成员压缩更依赖编译器和 ABI，尤其不能根据示意图断言实际布局。

这里涉及两个不同概念：

- 对齐（alignment）：对象起始地址需要满足的倍数要求；
- 填充（padding）：编译器为满足对齐和布局规则插入、不属于业务字段的空间。

`[[no_unique_address]]` 不会取消类型的对齐要求，也不是通用的“结构体压缩”开关。

## 8. 相同空类型成员的限制直觉

不要认为加了属性后任意数量的相同空成员都能无条件压到一个地址：

```cpp
struct Empty {};

struct TwoEmpties {
    [[no_unique_address]] Empty first;
    [[no_unique_address]] Empty second;
    int value{};
};
```

`first` 和 `second` 是两个相同类型的不同子对象，语言的对象身份和地址规则会限制它们完全重合。最终大小仍取决于实现。该属性应理解为“给布局器更多合法选择”，不是“把成员大小强制变成 0”。

## 9. 在标准库和企业代码中的典型用途

常见场景包括：

- 无状态 allocator（分配器）；
- `unique_ptr` 中的空 deleter（删除器）；
- 泛型容器中的 comparator（比较器）；
- policy-based design（策略类设计）；
- lambda 和函数对象包装；
- ranges、迭代器及 view 内部保存的无状态操作对象。

例如，一个删除器可能没有数据，却仍必须作为对象的一部分参与类型系统和调用：

```cpp
struct DefaultDelete {
    void operator()(int* pointer) const {
        delete pointer;
    }
};

class Owner {
private:
    int* pointer_{};
    [[no_unique_address]] DefaultDelete deleter_;
};
```

`deleter_` 有明确的组合语义，但在合适实现上可能不增加 Owner 的大小。这也是现代 C++ 更偏好“组合 + `[[no_unique_address]]`”而不是单纯为了布局去继承空类的原因之一。

## 10. 如何运行现有配套程序

项目中的 [`item39_private_inheritance.cpp`](../code/item_32to40/item39_private_inheritance.cpp) 同时比较普通成员、空基类和 `[[no_unique_address]]` 成员。

因为该属性属于 C++20，编译时使用：

```bash
cd /home/jason/study/ck_study/cplusplus/effective_c++/code/item_32to40
g++ -std=c++20 -Wall -Wextra -Wpedantic -g -O0 \
    item39_private_inheritance.cpp -o /tmp/item39
/tmp/item39
```

在一种常见环境中可能看到：

```text
samples=2
sizeof empty/member/base: 1/8/4
sizeof no_unique_address member: 4
```

正确的观察方式是比较相对关系：

1. 普通空成员是否引入了额外空间和填充；
2. 空基类是否利用了 EBO；
3. 带属性的成员是否得到与 EBO 类似的压缩；
4. 更换编译器或架构后，结果是否变化。

不要把示例中的 `1/8/4/4` 当作所有机器都必须输出的标准答案。

## 总结

记住下面这条因果链即可：

```text
空类也需要对象身份
    → 独立空对象大小不能为 0
    → 空类作为普通成员可能引入空间和对齐填充
    → EBO 可压缩空基类，但会使用继承关系
    → C++20 [[no_unique_address]] 让成员组合也有类似优化机会
    → 它只允许布局复用，不保证 sizeof 必然减小
```

选择它的第一理由应当是准确表达“这个策略成员可能没有状态”，空间收益需要在目标编译器和 ABI 上实际测量。
