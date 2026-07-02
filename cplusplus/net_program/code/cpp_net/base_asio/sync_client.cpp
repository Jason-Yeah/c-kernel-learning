/**
 * Create an endpoint based on the server's IP address and port,
 * then create a socket to connect to that endpoint; subsequently,
 * we can send and receive data using synchronous read and write operations.
 */

#include <boost/asio.hpp>
#include <iostream>

const int MAX_LENGTH = 1024;

int main()
{
    try
    {
        // Create IO_context
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::endpoint remote_ep(
            boost::asio::ip::make_address("127.0.0.1"), 9190);
        boost::asio::ip::tcp::socket sock(ioc, remote_ep.protocol());
        boost::system::error_code ec = boost::asio::error::host_not_found;
        sock.connect(remote_ep, ec);
        if (ec)
        {
            std::cout << "Failed to parse the IP address. Error code = "
                      << ec.value() << ". Message is: " << ec.message()
                      << std::endl;
            return ec.value();
        }

        std::cout << "Enter massage: ";
        char request[MAX_LENGTH];
        std::cin.getline(request, MAX_LENGTH); // Enter by line.
        size_t request_len = strlen(request);
        // Send everything at once
        boost::asio::write(sock, boost::asio::buffer(request, request_len));

        // ECHO SERVICE
        char reply[MAX_LENGTH];
        size_t reply_len = 0;
        reply_len =
            boost::asio::read(sock, boost::asio::buffer(reply, request_len));
        std::cout << "Reply is: ";
        std::cout.write(reply, reply_len) << std::endl;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }

    return 0;
}