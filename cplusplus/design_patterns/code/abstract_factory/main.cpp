#include "app.hpp"

int main()
{
    // 缺点，这里还是写死了Windows
    Application winapp(std::make_unique<WinFactory>());
    winapp.run();

    Application linuxapp(std::make_unique<LinuxFactory>());
    linuxapp.run();

    puts("TYPE 2");

    auto factory = GUIFactoryCreator::create("Linux");
    Application app(std::move(factory));
    app.run();

    puts("TYPE 2");

    init_factory_register();

    auto fac = FactoryRegister::create("win");
    Application app2(std::move(fac));

    app2.run();

    auto f = FactoryRegister::create("win");

    // 不用注册了，但不建议用
    Application app3(std::move(f));
    app3.run();

    return 0;
}
