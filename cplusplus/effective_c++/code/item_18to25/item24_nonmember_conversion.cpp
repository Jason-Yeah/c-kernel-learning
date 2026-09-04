#include <iostream>
#include <numeric>
#include <stdexcept>

class Rational
{
public:
    // 此处故意不加 explicit：允许 int 自动变为 Rational(n, 1)。
    Rational(int numerator = 0, int denominator = 1)
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

std::ostream &operator<<(std::ostream &output, const Rational &value)
{
    return output << value.numerator() << '/' << value.denominator();
}

int main()
{
    Rational half{1, 2};
    std::cout << half * 2 << '\n'; // 2 转为 Rational(2, 1)
    std::cout << 2 * half
              << '\n'; // 左右两侧都允许转换，因为 operator* 是 non-member
}
