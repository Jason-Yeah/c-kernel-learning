#include "flyweight.hpp"

int main()
{
    CharFactory factory;
    std::string text = "hello hello hello"; // 15 个字符

    int x = 0;
    for (char ch : text)
    {
        if (ch == ' ')
            continue;

        Character *c = factory.GetFlyWeight(ch);
        c->Render(x, 0, 12);
        x += 10;
    }

    std::cout << "\n对象池大小: " << factory.PoolSize() << " 个"
              << "（'h','e','l','o' 共 4 个字符被反复复用！）" << std::endl;

    return 0;
}
