#include <iostream>
#include <stdexcept>

template <typename Number>
class Rational {
public:
    // 故意不写 explicit，使整数可以转换为 Rational<Number>。
    Rational(Number numerator, Number denominator = Number{1})
        : numerator_(numerator), denominator_(denominator)
    {
        if (denominator_ == Number{}) {
            throw std::invalid_argument("denominator cannot be zero");
        }
    }

    // 每次 Rational<Number> 实例化都会生成一个对应的普通非成员函数。
    // 它能由 ADL 找到，而且两侧实参都允许进行用户自定义转换。
    friend Rational operator*(const Rational &left, const Rational &right)
    {
        return {left.numerator_ * right.numerator_,
                left.denominator_ * right.denominator_};
    }

    friend std::ostream &operator<<(std::ostream &output, const Rational &value)
    {
        return output << value.numerator_ << '/' << value.denominator_;
    }

private:
    Number numerator_;
    Number denominator_;
};

int main()
{
    Rational<int> half{1, 2};

    std::cout << "half * 2 = " << half * 2 << '\n';
    std::cout << "2 * half = " << 2 * half << '\n';
}
