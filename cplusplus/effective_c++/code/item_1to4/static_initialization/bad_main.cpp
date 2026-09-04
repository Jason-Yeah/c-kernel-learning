#include "bad_logger.h"

#include <iostream>

int main()
{
    std::cout << "[main] logger ready: " << std::boolalpha << loggerIsReady
              << ", config initialized: " << configWasInitialized << '\n';
}
