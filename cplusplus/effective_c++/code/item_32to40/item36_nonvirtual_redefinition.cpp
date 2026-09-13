#include <iostream>
struct Base
{
    virtual ~Base() = default;
    void identify() const { std::cout << "Base::identify\n"; }
    virtual void describe() const { std::cout << "Base::describe\n"; }
};

struct Derived final : Base
{
    void identify() const
    {
        std::cout << "Derived::identify\n";
    } // 隐藏，不是覆盖。
    void describe() const override { std::cout << "Derived::describe\n"; }
};

int main()
{
    Derived d;
    Base &b = d; // 同一个对象，另一个静态类型的访问路径。
    d.identify();
    b.identify();
    d.describe();
    b.describe();
}
