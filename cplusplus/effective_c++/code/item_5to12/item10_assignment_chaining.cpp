#include <iostream>

class Counter
{
public:
    explicit Counter(int value = 0) : value_(value) {}

    Counter &operator=(const Counter &rhs)
    {
        value_ = rhs.value_;
        return *this;
    }

    Counter &operator+=(int delta)
    {
        value_ += delta;
        return *this;
    }

    int value() const { return value_; }

private:
    int value_{};
};

int main()
{
    Counter first{1};
    Counter second{2};
    Counter third{3};

    first = second = third;            // 等价于 first = (second = third)
    Counter &sameFirst = (first += 4); // 返回的是 first 本身的引用
    sameFirst += 1;

    std::cout << first.value() << ", " << second.value() << ", "
              << third.value() << '\n';
}
