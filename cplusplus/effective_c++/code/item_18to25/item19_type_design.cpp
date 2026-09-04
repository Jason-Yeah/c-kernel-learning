#include <cstdlib>
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
            throw std::invalid_argument("denominator cannot be zero");
        normalize();
    }

    int numerator() const noexcept { return numerator_; }
    int denominator() const noexcept { return denominator_; }

private:
    void normalize()
    {
        if (denominator_ < 0)
        {
            numerator_ = -numerator_;
            denominator_ = -denominator_;
        }
        const int divisor = std::gcd(numerator_, denominator_);
        numerator_ /= divisor;
        denominator_ /= divisor;
    }

    int numerator_{};
    int denominator_{1};
};

int main()
{
    Rational value{-6, -8};
    std::cout << value.numerator() << '/' << value.denominator() << '\n'; // 3/4

    try
    {
        Rational invalid{1, 0};
        (void)invalid;
    }
    catch (const std::invalid_argument &error)
    {
        std::cout << "rejected: " << error.what() << '\n';
    }
}
