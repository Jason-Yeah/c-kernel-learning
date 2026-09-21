#include <cstddef>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>

class Widget
{
public:
    explicit Widget(bool shouldThrow) : state_(shouldThrow ? -1 : 1)
    {
        std::cout << "Widget constructor\n";
        if (shouldThrow)
        {
            throw std::runtime_error("constructor failed");
        }
    }

    ~Widget() { std::cout << "Widget destructor\n"; }

    static void *operator new(std::size_t size) { return ::operator new(size); }

    static void operator delete(void *memory) noexcept
    {
        std::cout << "ordinary delete\n";
        ::operator delete(memory);
    }

    static void *operator new(std::size_t size, std::ostream &log)
    {
        log << "logging placement new: " << size << " byte(s)\n";
        return ::operator new(size);
    }

    // 附加参数与上面的 placement new 一致。只有构造失败时，
    // new 表达式才会自动调用这个匹配的 placement delete。
    static void operator delete(void *memory, std::ostream &log) noexcept
    {
        log << "matching placement delete after constructor failure\n";
        ::operator delete(memory);
    }

private:
    int state_;
};

class BufferObject
{
public:
    explicit BufferObject(int value) : value_(value)
    {
        std::cout << "BufferObject constructed: " << value_ << '\n';
    }

    ~BufferObject() { std::cout << "BufferObject destroyed\n"; }

private:
    int value_;
};

int main()
{
    try
    {
        Widget *widget = new (std::cout) Widget{true};
        (void)widget;
    }
    catch (const std::exception &error)
    {
        std::cout << "caught: " << error.what() << '\n';
    }

    // 构造成功后，普通 delete 没有保存最初的 log 参数，因而调用普通
    // operator delete，而不是上面的 placement delete。
    Widget *successfulWidget = new (std::cout) Widget{false};
    delete successfulWidget;

    // storage 只提供内存，construct_at 才在其中开始 BufferObject 生命周期。
    alignas(BufferObject) std::byte storage[sizeof(BufferObject)];
    auto *bufferObject =
        std::construct_at(reinterpret_cast<BufferObject *>(storage), 42);
    std::destroy_at(bufferObject);
    // 不可 delete bufferObject：底层存储来自栈上数组，不是 operator new。
}
