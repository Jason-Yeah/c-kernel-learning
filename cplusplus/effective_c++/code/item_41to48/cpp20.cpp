#include <concepts>
#include <iostream>
#include <type_traits>

// ============================================================
// C++20 concept：限制 Rational 只能使用数字类型
// ============================================================

template <typename T>
concept Number = std::integral<T> || std::floating_point<T>;

// ============================================================
// Item 43：模板化基类
// ============================================================

template <Number T> class RationalBase
{
protected:
    void debug_impl(T n, T d) const
    {
        std::cout << "value = " << n << "/" << d << '\n';
    }
};

// ============================================================
// Rational
// ============================================================

template <Number T> class Rational : private RationalBase<T>
{
public:
    // 故意不写 explicit
    // 这样 int 可以隐式转换成 Rational<int>
    Rational(T n = 0, T d = 1) : n_(n), d_(d) {}

    // ========================================================
    // Item 45：
    // member function template
    //
    // Rational<int> 可以构造 Rational<double>
    // 前提：U 能转换成 T
    // ========================================================

    template <Number U>
        requires std::convertible_to<U, T>
    Rational(const Rational<U> &other)
        : n_(static_cast<T>(other.numerator())),
          d_(static_cast<T>(other.denominator()))
    {
    }

    T numerator() const { return n_; }

    T denominator() const { return d_; }

    // ========================================================
    // Item 43：
    // RationalBase<T> 是 dependent base
    //
    // 所以使用 this->
    // ========================================================

    void debug() const { this->debug_impl(n_, d_); }

    // ========================================================
    // Item 46：
    //
    // 在类模板里定义非模板 friend
    //
    // Rational<int> 实例化后得到具体普通函数：
    //
    // operator+(Rational<int>, Rational<int>)
    //
    // 因此 2 可以隐式变成 Rational<int>(2)
    // ========================================================

    friend Rational operator+(const Rational &lhs, const Rational &rhs)
    {
        return Rational(lhs.n_ * rhs.d_ + rhs.n_ * lhs.d_, lhs.d_ * rhs.d_);
    }

    // ========================================================
    // Item 47 / 48 + C++17/20现代写法：
    //
    // if constexpr
    // 编译期根据 T 选择代码
    // ========================================================

    void describe() const
    {
        if constexpr (std::integral<T>)
        {
            std::cout << "integral Rational\n";
        }
        else if constexpr (std::floating_point<T>)
        {
            std::cout << "floating-point Rational\n";
        }
    }

private:
    T n_;
    T d_;
};

int main()
{
    Rational<int> a(1, 2);

    // Item 43
    a.debug();

    // Item 46：
    // 2 自动转换成 Rational<int>(2,1)
    auto b = a + 2;

    b.debug();

    // Item 45：
    // Rational<int> -> Rational<double>
    Rational<double> c = a;

    c.debug();

    // Item 47 / 48：
    // 编译期判断类型
    a.describe();
    c.describe();

    // concept 限制：
    // Rational<std::string> x;   // 编译失败
}