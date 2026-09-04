#include <iostream>

namespace
{
    bool initializeLogger()
    {
        std::cout << "[logger] initialize logger\n";
        return true;
    }
} // namespace

// 首次调用时才初始化；C++11 起并发的首次调用也只会完成一次初始化。
bool &loggerIsReady()
{
    static bool ready = initializeLogger();
    return ready;
}

namespace
{

bool initializeConfig()
{
    const bool loggerReady = loggerIsReady();
    std::cout << "[config] logger ready while config initializes: "
              << std::boolalpha << loggerReady << '\n';
    return true;
}

} // namespace

bool &configIsReady()
{
    static bool ready = initializeConfig();
    return ready;
}

int main()
{
    std::cout << "[main] request config\n";
    configIsReady();
}
