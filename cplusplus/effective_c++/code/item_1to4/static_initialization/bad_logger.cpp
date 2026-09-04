#include "bad_logger.h"

#include <iostream>

namespace
{
bool initializeLogger()
{
    std::cout << "[logger] initialize logger\n";
    return true;
}
} // namespace

// 这是动态初始化，而不是常量初始化。
bool loggerIsReady = initializeLogger();
