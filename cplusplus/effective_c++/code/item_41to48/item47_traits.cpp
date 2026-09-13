#include <iostream>
#include <iterator>
#include <list>
#include <vector>

template <typename Iterator, typename Distance>
void advanceByImpl(Iterator &position, Distance distance,
                   std::random_access_iterator_tag)
{
    std::cout << "random-access path: one += operation\n";
    position += distance;
}

template <typename Iterator, typename Distance>
void advanceByImpl(Iterator &position, Distance distance,
                   std::input_iterator_tag)
{
    std::cout << "linear path: repeated ++ operations\n";
    while (distance > 0) {
        ++position;
        --distance;
    }
}

template <typename Iterator, typename Distance>
void advanceBy(Iterator &position, Distance distance)
{
    // traits 把迭代器类型映射为一个“类别标签类型”。重载解析再根据标签
    // 在编译期选择算法；这里没有运行期 if，也没有虚函数。
    using Category =
        typename std::iterator_traits<Iterator>::iterator_category;
    advanceByImpl(position, distance, Category{});
}

int main()
{
    std::vector<int> numbers{10, 20, 30, 40};
    auto vectorPosition = numbers.begin();
    advanceBy(vectorPosition, 2);
    std::cout << "vector value: " << *vectorPosition << '\n';

    std::list<int> linkedNumbers{10, 20, 30, 40};
    auto listPosition = linkedNumbers.begin();
    advanceBy(listPosition, 2);
    std::cout << "list value: " << *listPosition << '\n';
}
