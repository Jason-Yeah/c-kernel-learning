#include <boost/asio.hpp>
#include <iostream>

const int MAX_LENGTH = 1024;

int main(int argc, char *argv[])
{
    unsigned short port = std::stoi(argv[1]);
    try
    {
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::endpoint remote_ep(
            boost::asio::ip::make_address("127.0.0.1"), port);
        boost::asio::ip::tcp::socket sock(ioc);
        boost::system::error_code ec = boost::asio::error::host_not_found;

        sock.connect(remote_ep, ec);

        if (ec)
        {
            std::cout << "Connect failed, error code is: " << ec.value()
                      << ". err message is: " << ec.message() << std::endl;
            return ec.value();
        }

        std::cout << "Enter Message: ";
        char request[MAX_LENGTH];
        std::cin.getline(request, MAX_LENGTH);
        size_t req_len = strlen(request);
        boost::asio::write(sock, boost::asio::buffer(request, req_len));

        char replay[MAX_LENGTH];
        size_t replay_len = 0;
        replay_len =
            boost::asio::read(sock, boost::asio::buffer(replay, req_len));

        // ECHO
        std::cout << "Reply is: ";
        std::cout.write(replay, replay_len) << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}