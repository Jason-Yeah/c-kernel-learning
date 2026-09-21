#include <charconv>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

[[nodiscard]] std::optional<int> parseInteger(std::string_view text)
{
    int value{};
    const char *const begin = text.data();
    const char *const end = begin + text.size();
    const auto [position, error] = std::from_chars(begin, end, value);

    if (error != std::errc{} || position != end) {
        return std::nullopt;
    }
    return value;
}

class Processor {
public:
    virtual ~Processor() = default;
    virtual void process(int value) const
    {
        std::cout << "Processor: " << value << '\n';
    }
};

class DoublingProcessor final : public Processor {
public:
    void process(int value) const override
    {
        std::cout << "DoublingProcessor: " << value * 2 << '\n';
    }
};

#ifdef ENABLE_WARNING_DEMO
void warningExamples()
{
    // 1. 丢弃 [[nodiscard]] 结果。
    parseInteger("123");

    // 2. 有符号负数与无符号 size 比较，-1 会转换为很大的无符号值。
    const int index = -1;
    const std::vector<int> values{10, 20, 30};
    if (index < values.size()) {
        std::cout << values.front() << '\n';
    }

    // 3. 内层变量隐藏外层同名变量。
    int total = 10;
    {
        int total = 20;
        std::cout << total << '\n';
    }
    std::cout << total << '\n';
}
#endif

int main()
{
    const auto parsed = parseInteger("42");
    if (!parsed) {
        std::cerr << "invalid integer\n";
        return 1;
    }

    DoublingProcessor processor;
    const Processor &interface = processor;
    interface.process(*parsed);

    const int index = -1;
    const std::vector<int> values{10, 20, 30};
    std::cout << std::boolalpha
              << "index is before size: "
              << std::cmp_less(index, values.size()) << '\n';

#ifdef ENABLE_WARNING_DEMO
    warningExamples();
#endif
}
