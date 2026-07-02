#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <set>
#include <thread>

const int MAX_SIZE = 1024;

using sock_p = std::shared_ptr<boost::asio::ip::tcp::socket>;

std::set<std::shared_ptr<std::thread>> thread_set;

void session(sock_p sock)
{
    try
    {
        while (1)
        {
            char data[MAX_SIZE];
            // memset(data, '\0', sizeof(data));
            boost::system::error_code ec;
            // Not accounting for packet coalescing.
            size_t len =
                sock->read_some(boost::asio::buffer(data, MAX_SIZE), ec);
            // size_t len = boost::asio::read(*sock, boost::asio::buffer(data,
            // MAX_SIZE), ec);

            if (ec == boost::asio::error::eof)
            {
                std::cout << "connection closed by peer" << std::endl;
                break;
            }
            else if (ec)
            {
                throw boost::system::system_error(ec);
            }
            std::cout << "Receive from "
                      << sock->remote_endpoint().address().to_string()
                      << std::endl;
            std::cout << "Receive message is ";
            std::cout.write(
                data,
                len); // If the message has '\0', the "std::cout" will stop;
            std::cout << std::endl;
            // ECHO SERVICE
            boost::asio::write(*sock, boost::asio::buffer(data, len));
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n" << std::endl;
    }
}

void server(boost::asio::io_context &ioc, unsigned short port)
{
    boost::asio::ip::tcp::acceptor acceptor(
        ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    while (1)
    {
        sock_p clnt_sock(new boost::asio::ip::tcp::socket(ioc));
        acceptor.accept(*clnt_sock);
        auto thd = std::make_shared<std::thread>(session, clnt_sock);
        thread_set.insert(thd); // Prevent premature destruction
    }
}

int main()
{
    try
    {
        boost::asio::io_context ioc;
        server(ioc, 9190);
        // Waiting other working threads when main thread prepares to quit...
        for (auto &thread : thread_set)
            thread->join();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n" << std::endl;
    }
    return 0;
}