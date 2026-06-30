#include "endpoint.h"
#include <boost/asio.hpp>
#include <iostream>

// using namespace boost;

int client_end_point()
{
    std::string raw_ip_addr = "127.4.8.1";
    unsigned short port = 9190;

    boost::system::error_code ec;
    boost::asio::ip::address ip_addr =
        boost::asio::ip::make_address(raw_ip_addr, ec);
    if (ec.value()) // not zero
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }

    // create_endpoint + bind(ip, port);
    boost::asio::ip::tcp::endpoint ep(ip_addr, port);

    return 0;
}

int server_end_point()
{
    unsigned short port = 9190;
    boost::asio::ip::address ip_addr = boost::asio::ip::address_v4::any();

    // TCP communication endpoint
    boost::asio::ip::tcp::endpoint ep(ip_addr, port);
    return 0;
}

int create_tcp_socket()
{
    // parameter IO_CONTEXT used to communicate...
    // (总调度、定时器、socket分配资源)
    boost::asio::io_context ioc;
    boost::asio::ip::tcp protocol = boost::asio::ip::tcp::v4();

    boost::asio::ip::tcp::socket sock(ioc);

    boost::system::error_code ec;
    sock.open(protocol, ec);
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }

    return 0;
}

int create_acceptor_socket()
{
    boost::asio::io_context ioc;
    /*
    boost::asio::ip::tcp::acceptor acceptor(ioc);

    boost::asio::ip::tcp protocol = boost::asio::ip::tcp::v4();
    boost::system::error_code ec;
    acceptor.open(protocol, ec);
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }
    */

    // socket + bind + listen
    boost::asio::ip::tcp::acceptor acceptor(
        ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9190));
    return 0;
}

int bind_acceptor_socket()
{
    unsigned short port = 9190;
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port);

    boost::asio::io_context ioc;
    boost::asio::ip::tcp::acceptor acceptor(ioc, ep.protocol());
    boost::system::error_code ec;

    acceptor.bind(ep, ec);
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }

    return 0;
}

int connect_to_end()
{
    std::string raw_ip_addr = "192.168.1.124";
    unsigned short port = 9190;

    try
    {
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(raw_ip_addr), port);
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket sock(ioc, ep.protocol());
        sock.connect(ep);

        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}

int dns_connect_to_end()
{
    std::string host = "llfc.club";
    unsigned short port = 9190;
    boost::asio::io_context ioc;
    // 域名解析
    boost::asio::ip::tcp::resolver resolver(ioc);

    try
    {
        // gethostbyname(char* str);
        auto endpoints = resolver.resolve(host, std::to_string(port));
        auto ep = *endpoints.begin();
        // ep = endpoints.front();

        boost::asio::ip::tcp::socket sock(ioc);
        boost::asio::connect(sock, endpoints);

        std::cout << "Connected to: " << ep.endpoint() << std::endl;
        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}

int accept_new_connection()
{
    const int BACKLOG_SIZE = 30;
    unsigned short port = 9190;
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port);
    boost::asio::io_context ioc;

    try
    {
        boost::asio::ip::tcp::acceptor acceptor(ioc, ep.protocol());
        acceptor.bind(ep);
        acceptor.listen(BACKLOG_SIZE);
        boost::asio::ip::tcp::socket sock(ioc);

        // 用于和客户端通信 int conn = accept(fd, NULL, NULL);
        acceptor.accept(sock);

        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}
