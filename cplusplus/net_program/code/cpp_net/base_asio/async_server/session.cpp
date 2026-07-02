#include "session.hpp"
#include <iostream>

session::session(boost::asio::io_context &ioc) : _sock(ioc) {}

boost::asio::ip::tcp::socket &session::get_socket() { return _sock; }

// Listen for client data reads
void session::start()
{
    // memset(_data, '\0', MAX_LENGTH);
    auto self = shared_from_this();
    _sock.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                          [self](const boost::system::error_code &ec,
                                 std::size_t bytes_transfered)
                          { self->handle_read(ec, bytes_transfered); });
}

// ECHO SERVICE
void session::handle_read(const boost::system::error_code &ec,
                          std::size_t bytes_transfered)
{
    if (!ec)
    {
        std::cout << "Server receive data is: ";
        std::cout.write(_data, bytes_transfered) << std::endl;
        auto self = shared_from_this();
        boost::asio::async_write(
            // Attention: Send exactly the amount of data received.
            _sock, boost::asio::buffer(_data, bytes_transfered),
            [self](const boost::system::error_code &ec,
                   std::size_t bytes_transfered) { self->handle_write(ec); });
    }
    else
    {
        std::cout << "READ ERROR" << std::endl;
        // delete this; // delete this session_obj;
        boost::system::error_code ignore_ec;
        _sock.close(ignore_ec);
    }
}

// ECHO SERVICE
void session::handle_write(const boost::system::error_code &ec)
{
    if (!ec)
    {
        // memset(_data, 0, MAX_LENGTH);
        auto self = shared_from_this();
        // _sock.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
        //                       [self](const boost::system::error_code &ec,
        //                              std::size_t bytes_transfered)
        //                       { self->handle_read(ec, bytes_transfered); });
        start();
    }
    else
    {
        std::cout << "READ ERROR: " << ec.message() << std::endl;
        boost::system::error_code ignore_ec;
        _sock.close(ignore_ec);
    }
}
