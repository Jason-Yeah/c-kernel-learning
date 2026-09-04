#include "bad_logger.h"

#include <iostream>

namespace
{
bool initializeConfig()
{
    std::cout << "[config] logger ready while config initializes: "
              << std::boolalpha << loggerIsReady << '\n';
    return true;
}
} // namespace

// 它读取另一个 .cpp 中、动态初始化的变量。顺序未指定。
bool configWasInitialized = initializeConfig();
