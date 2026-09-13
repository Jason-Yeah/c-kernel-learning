#include <cassert>
#include <iostream>

namespace ordinary
{
struct Device
{
    explicit Device(int value) : id(value) {}
    int id;
};
struct Scanner : Device
{
    Scanner() : Device(1) {}
};
struct Printer : Device
{
    Printer() : Device(2) {}
};
struct Copier : Scanner, Printer
{
};
} // namespace ordinary
namespace shared
{
struct Device
{
    explicit Device(int value) : id(value) {}
    int id;
};
struct Scanner : virtual Device
{
    Scanner() : Device(1) {}
};
struct Printer : virtual Device
{
    Printer() : Device(2) {}
};
// 构造 Copier 时，只有这里的 Device(42) 初始化共享虚基类。
struct Copier : Scanner, Printer
{
    Copier() : Device(42) {}
};
} // namespace shared
struct Readable
{
    virtual ~Readable() = default;
    virtual void read() const = 0;
};
struct Writable
{
    virtual ~Writable() = default;
    virtual void write() const = 0;
};
struct MemoryDevice final : Readable, Writable
{
    void read() const override { std::cout << "read interface\n"; }
    void write() const override { std::cout << "write interface\n"; }
};
int main()
{
    ordinary::Copier a;
    ordinary::Device *left = static_cast<ordinary::Scanner *>(&a);
    ordinary::Device *right = static_cast<ordinary::Printer *>(&a);
    std::cout << "ordinary ids: " << left->id << ", " << right->id << '\n';
    std::cout << "ordinary same base: " << std::boolalpha << (left == right)
              << '\n';
    assert(left != right && left->id == 1 && right->id == 2);
    // ordinary::Device* ambiguous = &a; // 两份基类，转换有歧义。

    shared::Copier b;
    shared::Device *sharedLeft = static_cast<shared::Scanner *>(&b);
    shared::Device *sharedRight = static_cast<shared::Printer *>(&b);
    std::cout << "virtual id: " << sharedLeft->id << '\n';
    std::cout << "virtual same base: " << (sharedLeft == sharedRight) << '\n';
    assert(sharedLeft == sharedRight && sharedLeft->id == 42);

    MemoryDevice device;
    const Readable &reader = device;
    const Writable &writer = device;
    reader.read();
    writer.write();
}
