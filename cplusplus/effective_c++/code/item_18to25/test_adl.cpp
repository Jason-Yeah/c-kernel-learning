#include <iostream>
#include <utility>

namespace MyLib
{

class Widget
{
public:
    explicit Widget(int value) : value_(value) {}

    void swap(Widget &other) noexcept
    {
        std::cout << ">>> Widget::swap member\n";

        using std::swap;
        swap(value_, other.value_);
    }

    int value() const noexcept { return value_; }

private:
    int value_;
};

// non-member swap，与 Widget 在同一个 namespace
void swap(Widget &lhs, Widget &rhs) noexcept
{
    std::cout << ">>> MyLib::swap non-member\n";

    lhs.swap(rhs);
}

} // namespace MyLib

template <typename T> void genericSwap(T &lhs, T &rhs) { std::swap(lhs, rhs); }

int main()
{
    MyLib::Widget a{10};
    MyLib::Widget b{20};

    genericSwap(a, b);

    std::cout << a.value() << '\n';
    std::cout << b.value() << '\n';
}