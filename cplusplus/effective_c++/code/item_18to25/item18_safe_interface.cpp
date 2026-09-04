#include <iostream>
#include <stdexcept>

enum class Month
{
    jan = 1,
    feb,
    mar,
    apr,
    may,
    jun,
    jul,
    aug,
    sep,
    oct,
    nov,
    dec
};

class Date
{
public:
    Date(int year, Month month, int day) : year_(year), month_(month), day_(day)
    {
        if (day_ < 1 || day_ > 31)
            throw std::out_of_range("day must be in [1, 31]");
    }

    // [[nodiscard]] 该返回值不应该被随意丢弃
    // 比如直接isInYear(2026); 没用返回值编译器会给出warning
    [[nodiscard("必须检查年份对不对")]] bool isInYear(int year) const noexcept
    {
        return year_ == year;
    }

private:
    int year_{};
    Month month_{};
    int day_{};
};

int main()
{
    Date meeting{2026, Month::aug, 3};
    /*
    meeting.isInYear(2027);
    item18_safe_interface.cpp: In function ‘int main()’:
    item18_safe_interface.cpp:45:21: warning: ignoring return value of ‘bool
    Date::isInYear(int) const’, declared with attribute ‘nodiscard’:
    ‘必须检查年份对不对’ [-Wunused-result] 45 |     meeting.isInYear(2027); |
    ~~~~~~~~~~~~~~~~^~~~~~ item18_safe_interface.cpp:31:46: note: declared here
    31 |     [[nodiscard("必须检查年份对不对")]] bool isInYear(int year) const
    noexcept
        |
    */

    std::cout << std::boolalpha << meeting.isInYear(2026) << '\n';

    try
    {
        Date invalid{2026, Month::feb, 40};
        (void)invalid; // 表示我就是不用这个表达式，故意的
    }
    catch (const std::out_of_range &error)
    {
        std::cout << "rejected: " << error.what() << '\n';
    }

    // Date wrong{2026, 9, 3}; // 编译错误：int 不能隐式变为 Month
}
