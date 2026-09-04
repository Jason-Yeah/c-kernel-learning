#include <iostream>
#include <numeric>
#include <stdexcept>

class Rational
{
public:
    Rational(int numerator, int denominator)
        : numerator_(numerator), denominator_(denominator)
    {
        if (denominator_ == 0)
            throw std::invalid_argument("zero denominator");
        const int divisor = std::gcd(numerator_, denominator_);
        numerator_ /= divisor;
        denominator_ /= divisor;
    }

    int numerator() const noexcept { return numerator_; }
    int denominator() const noexcept { return denominator_; }

private:
    int numerator_{};
    int denominator_{1};
};

Rational operator*(const Rational &left, const Rational &right)
{
    return Rational(left.numerator() * right.numerator(),
                    left.denominator() * right.denominator());
}

int main()
{
    Rational half{1, 2};
    Rational quarter = half * half; // 返回一个新对象的值，安全
    std::cout << quarter.numerator() << '/' << quarter.denominator() << '\n';

    // const Rational& temporary = half * half;
    // 这种“局部 const
    // 引用绑定临时对象”在当前作用域内是安全的：临时对象寿命会延长。 条款 21
    // 禁止的是函数把它自己的局部对象/临时对象引用返回给调用者。
}
