#include <iostream>
#include <string>
#include <vector>

template <typename Container> void printFirst(const Container &container)
{
    // Container 是模板参数，因此 const_iterator 是依赖名称。
    // typename 告诉编译器：后面的名称是一种类型，而不是静态数据成员。
    typename Container::const_iterator position = container.begin();

    if (position != container.end())
    {
        std::cout << *position << '\n';
    }
}

template <typename Container> using Element = typename Container::value_type;

int main()
{
    std::vector<std::string> words{"template", "typename"};
    printFirst(words);

    // <=> std::string
    // Element<std::vector<std::string>>
    // std::vector<std::string>>::value_type == std::string
    Element<decltype(words)> anotherWord = "dependent type";
    std::cout << anotherWord << '\n';
}
