#include "server.hpp"
#include "session.hpp"
#include <boost/asio.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
    unsigned short port = std::stoi(argv[1]);
    try
    {
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::endpoint remote_ep(
            boost::asio::ip::make_address("127.0.0.1"), port);

        auto sev = std::make_shared<server>(ioc, port);
        sev->run();
        ioc.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}