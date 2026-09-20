#include <iostream>

namespace NonExplicit
{

template <typename T> class Rational
{
public:
    Rational(T n = 0, T d = 1) : n_(n), d_(d)
    {
        std::cout << "NonExplicit::Rational ctor  " << n_ << "/" << d_ << '\n';
    }

    friend Rational operator*(const Rational &lhs, const Rational &rhs)
    {
        std::cout << "NonExplicit::operator*\n";

        return Rational(lhs.n_ * rhs.n_, lhs.d_ * rhs.d_);
    }

    void print() const { std::cout << n_ << "/" << d_ << '\n'; }

private:
    T n_;
    T d_;
};

} // namespace NonExplicit

namespace Explicit
{

template <typename T> class Rational
{
public:
    explicit Rational(T n = 0, T d = 1) : n_(n), d_(d)
    {
        std::cout << "Explicit::Rational ctor     " << n_ << "/" << d_ << '\n';
    }

    friend Rational operator*(const Rational &lhs, const Rational &rhs)
    {
        std::cout << "Explicit::operator*\n";

        return Rational(lhs.n_ * rhs.n_, lhs.d_ * rhs.d_);
    }

    void print() const { std::cout << n_ << "/" << d_ << '\n'; }

private:
    T n_;
    T d_;
};

} // namespace Explicit

int main()
{
    NonExplicit::Rational<int> a(1, 2);

    // 关键：
    // 这里的 2 会被偷偷转换成 Rational<int>(2, 1)
    auto x = 2 * a;

    x.print();

    Explicit::Rational<int> b(1, 2);

    // 这一句如果打开，会直接编译失败：
    // auto bad = b * 2;

    // 必须明确创建 Rational<int>
    auto y = b * Explicit::Rational<int>(2);

    y.print();
}