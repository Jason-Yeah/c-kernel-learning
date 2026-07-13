// main.cpp — 入口
#include "network_server.hpp"
#include <iostream>

int main()
{
    try {
        NetworkServer server(9190);
        server.run();
    } catch (std::exception& e) {
        std::cerr << "[main] fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
