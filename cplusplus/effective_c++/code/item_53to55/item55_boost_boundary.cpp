#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#if __has_include(<boost/multiprecision/cpp_int.hpp>)
#include <boost/multiprecision/cpp_int.hpp>
#define EFFECTIVE_CPP_HAS_BOOST_MULTIPRECISION 1
#else
#define EFFECTIVE_CPP_HAS_BOOST_MULTIPRECISION 0
#endif

namespace project_math {

// 调用者只依赖 std::string，不需要知道内部是否使用 Boost 类型。
[[nodiscard]] std::string factorialText(unsigned value)
{
#if EFFECTIVE_CPP_HAS_BOOST_MULTIPRECISION
    boost::multiprecision::cpp_int result = 1;
    for (unsigned factor = 2; factor <= value; ++factor) {
        result *= factor;
    }
    return result.convert_to<std::string>();
#else
    // 教学环境没有 Boost 时提供范围受限的回退实现。
    // 生产项目若要求任意精度，应让构建在缺少 Boost 时直接失败。
    if (value > 20) {
        throw std::out_of_range{
            "without Boost, this demo only supports factorial up to 20"};
    }

    std::uint64_t result = 1;
    for (unsigned factor = 2; factor <= value; ++factor) {
        result *= factor;
    }
    return std::to_string(result);
#endif
}

} // namespace project_math

int main()
{
#if EFFECTIVE_CPP_HAS_BOOST_MULTIPRECISION
    std::cout << "Boost.Multiprecision is available\n";
    std::cout << "100! = " << project_math::factorialText(100) << '\n';
#else
    std::cout << "Boost.Multiprecision is not installed; "
                 "using the bounded standard-library fallback\n";
    std::cout << "20! = " << project_math::factorialText(20) << '\n';
#endif
}
