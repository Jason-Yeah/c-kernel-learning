#include "singleton.hpp"

Singleton98 *Singleton98::inst_ = NULL;

int main()
{
    {
        using Singleton = Singleton98;
        Singleton *s1 = Singleton::GetInstance();
        Singleton *s2 = Singleton::GetInstance();
        Singleton *s3 = Singleton::GetInstance();

        s1->DoSomething();
        s2->DoSomething();

        // 三个指针指向同一个地址
        std::cout << "s1 == s2 == s3 ? "
                  << (s1 == s2 && s2 == s3 ? "是 ✅" : "否 ❌") << std::endl;
    }

    {
        using Singleton = Singleton11;
        Singleton &s1 = Singleton::GetInstance();
        Singleton &s2 = Singleton::GetInstance();
        s1.DoSomething();
        s2.DoSomething();

        std::cout << "s1 和 s2 是同一个对象? "
                  << (&s1 == &s2 ? "是 ✅" : "否 ❌") << std::endl;
    }

    ConfigManager::GetInstance().Set("theme", "dark");
    std::cout << ConfigManager::GetInstance().Get("theme") << std::endl;

    Logger::GetInstance().Log("系统启动");

    return 0;
}
