#include <array>
#include <cstddef>
#include <iostream>
#include <type_traits>

template <std::size_t N>
struct Factorial {
    static constexpr std::size_t value = N * Factorial<N - 1>::value;
};

template <>
struct Factorial<0> {
    static constexpr std::size_t value = 1;
};

constexpr std::size_t factorial(std::size_t number)
{
    std::size_t result = 1;
    for (std::size_t current = 2; current <= number; ++current) {
        result *= current;
    }
    return result;
}

template <typename T>
void describeNumber(T value)
{
    // if constexpr 只实例化被选中的分支；另一分支可含有对该 T 不成立的代码。
    if constexpr (std::is_integral_v<T>) {
        std::cout << value << " is an integral value\n";
    } else {
        std::cout << value << " is a non-integral value\n";
    }
}

static_assert(Factorial<5>::value == 120);
static_assert(factorial(6) == 720);

int main()
{
    // 编译期结果还可以成为类型的一部分，这里决定 std::array 的长度。
    std::array<int, Factorial<4>::value> values{};

    std::cout << "array size: " << values.size() << '\n';
    describeNumber(42);
    describeNumber(3.14);
}
