
#ifndef ASYNC_SERVER_SESSION_HPP
#define ASYNC_SERVER_SESSION_HPP

#include <boost/asio.hpp>
#include <memory>

class session : public std::enable_shared_from_this<session>
{
public:
    session(boost::asio::io_context &ioc);

    boost::asio::ip::tcp::socket &get_socket();

    void start();

private:
    // one or more times read
    void handle_read(const boost::system::error_code &ec,
                     std::size_t bytes_transfered);

    // only one time write the whole data
    void handle_write(const boost::system::error_code &ec);

private:
    boost::asio::ip::tcp::socket _sock;
    // enum {MAX_LENGTG = 1024}; //  C++03 永远合法，永远是编译期常量
    static constexpr std::size_t MAX_LENGTH = 1024; // C++11
    char _data[MAX_LENGTH];
};

#endif // ASYNC_SERVER_SESSION_HPP
