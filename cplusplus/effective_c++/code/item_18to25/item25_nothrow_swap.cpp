#include <iostream>
#include <memory>
#include <utility>

class Widget
{
public:
    explicit Widget(int value) : impl_(std::make_unique<Impl>(value)) {}

    void swap(Widget &other) noexcept
    {
        using std::swap;
        swap(impl_, other.impl_); // unique_ptr 的 swap 不抛异常，只交换指针
    }

    int value() const noexcept { return impl_->value; }

private:
    struct Impl
    {
        explicit Impl(int value) : value(value) {}
        int value{};
    };

    std::unique_ptr<Impl> impl_;
};

// 与 Widget 位于同一 namespace（这里是全局 namespace），供 ADL 找到。
void swap(Widget &left, Widget &right) noexcept { left.swap(right); }

template <class T>
void genericSwap(T &left, T &right) noexcept(noexcept(swap(left, right)))
{
    using std::swap;
    swap(left, right); // Widget 时 ADL 选择上面的高效版本
    // std::swap就不会ADL
    // 先using std::swap是吧std::swap加入当前作用域的候选
}

int main()
{
    Widget first{1};
    Widget second{2};
    genericSwap(first, second);
    std::cout << first.value() << ", " << second.value() << '\n';
}
