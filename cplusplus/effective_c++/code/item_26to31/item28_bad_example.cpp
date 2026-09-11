#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> titles{"first"};

    // 记录当前容量。
    const std::size_t oldCapacity = titles.capacity();

    // borrowed 直接引用 vector 内部的第一个 string 对象。
    const std::string &borrowed = titles[0];

    std::cout << "before reallocation: " << borrowed << '\n';
    std::cout << "old capacity: " << oldCapacity << '\n';

    // 添加元素，直到元素数量超过旧容量。
    // 超过 oldCapacity 时，vector 必须申请更大的内存。
    while (titles.size() <= oldCapacity)
    {
        titles.push_back("filler");
    }

    std::cout << "new capacity: " << titles.capacity() << '\n';
    std::cout << "current titles[0]: " << titles[0] << '\n';

    // 危险：borrowed 仍指向 vector 扩容前已经释放的旧内存。
    std::cout << "dangling reference: " << borrowed << '\n';
}