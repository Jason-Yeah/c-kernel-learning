#include <algorithm>
#include <iostream>
#include <numeric>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

[[nodiscard]] std::optional<double> average(std::span<const int> values)
{
    if (values.empty())
    {
        return std::nullopt;
    }

    const long long total = std::accumulate(values.begin(), values.end(), 0LL);
    return static_cast<double>(total) / static_cast<double>(values.size());
}

int main()
{
    // vector 拥有元素。span 和后面的 view 都只借用这些元素。
    std::vector<int> scores{72, -1, 95, 58, 83, 61};

    // 标准算法直接表达“删除非法负值”和“排序”的意图。
    std::erase_if(scores, [](int score) { return score < 0; });
    std::ranges::sort(scores);

    const std::span<const int> allScores{scores};
    if (const auto result = average(allScores))
    {
        std::cout << "average: " << *result << '\n';
    }

    // filter 是惰性的非拥有 view；遍历时才判断 score >= 60。
    // 不写 const：filter_view::begin() 可能需要缓存第一个匹配位置，
    // 因而标准 filter_view 不保证能通过 const view 调用 begin()。
    auto passingScores =
        scores | std::views::filter([](int score) { return score >= 60; });

    std::cout << "passing scores:";
    for (const int score : passingScores)
    {
        std::cout << ' ' << score;
    }
    std::cout << '\n';

    // 在 allScores/passingScores 使用期间，不要进行可能令 vector
    // 重新分配的修改，否则它们内部保存的访问状态可能失效。
}
