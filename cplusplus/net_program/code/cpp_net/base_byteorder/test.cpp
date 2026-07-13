// g++ -std=c++20 test.cpp

#include <cstdint>
#include <iostream>
#include <thread>

int main()
{
    uint32_t test = 0x01020304;
    auto *bytes = (const uint8_t *)&test;
    if (bytes[0] == 0x04)
        printf("小端\n");
    else
        puts("大端");

    if constexpr (std::endian::native == std::endian::little)
        puts("小端");

    auto thread_max = std::thread::hardware_concurrency();
    std::cout << thread_max << std::endl;

    return 0;
}