#include "session.hpp"
#include "server.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>

msg_node::msg_node(char *msg, int max_len)
    : _data(msg, max_len), _cur_len(0), _max_len(max_len)
{
}

msg_node::~msg_node() = default;

session::session(boost::asio::io_context &ioc, server *sev)
    : _sock(ioc), _server(sev)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
}

std::string &session::get_uuid() { return _uuid; }

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
// void session::handle_read(const boost::system::error_code &ec,
//                           std::size_t bytes_transfered)
// {
//     if (!ec)
//     {
//         std::cout << "Server receive data is: ";
//         std::cout.write(_data, bytes_transfered) << std::endl;
//         auto self = shared_from_this();
//         boost::asio::async_write(
//             // Attention: Send exactly the amount of data received.
//             _sock, boost::asio::buffer(_data, bytes_transfered),
//             [self](const boost::system::error_code &ec,
//                    std::size_t bytes_transfered) { self->handle_write(ec); });
//     }
//     else
//     {
//         std::cout << "READ ERROR" << std::endl;
//         // delete this; // delete this session_obj;
//         // boost::system::error_code ignore_ec;
//         // _sock.close(ignore_ec);
//         _server->clear_session(_uuid);
//     }
// }

void session::handle_read(const boost::system::error_code &ec,
                          std::size_t bytes_transfered)
{
    if (!ec)
    {
        std::cout << "Server receive data is: ";
        std::cout.write(_data, bytes_transfered) << std::endl;
        this->send(_data, bytes_transfered);
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
        // boost::system::error_code ignore_ec;
        // _sock.close(ignore_ec);
        _server->clear_session(_uuid);
    }
}

// ECHO SERVICE
// void session::handle_write(const boost::system::error_code &ec)
// {
//     if (!ec)
//     {
//         // memset(_data, 0, MAX_LENGTH);
//         auto self = shared_from_this();
//         // _sock.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
//         //                       [self](const boost::system::error_code &ec,
//         //                              std::size_t bytes_transfered)
//         //                       { self->handle_read(ec, bytes_transfered);
//         }); start();
//     }
//     else
//     {
//         std::cout << "READ ERROR: " << ec.message() << std::endl;
//         // boost::system::error_code ignore_ec;
//         // _sock.close(ignore_ec);
//         _server->clear_session(_uuid);
//     }
// }

void session::handle_write(const boost::system::error_code &ec)
{
    if (!ec)
    {
        std::shared_ptr<msg_node> next_node;
        {
            std::lock_guard<std::mutex> lock(_send_mtx);
            _send_queue.pop();
            if (!_send_queue.empty())
                next_node = _send_queue.front();
        }

        if (!next_node)
            return;

        auto self = shared_from_this();
        boost::asio::async_write(
            _sock, boost::asio::buffer(next_node->data(), next_node->size()),
            [self, next_node](const boost::system::error_code &ec,
                              std::size_t bytes_transferred)
            { self->handle_write(ec); });
    }
    else
    {
        std::cout << "WRITE ERROR: " << ec.message() << std::endl;
        _server->clear_session(_uuid);
    }
}

void session::send(char *msg, int max_len)
{
    bool pending = false;
    std::shared_ptr<msg_node> node;
    {
        std::lock_guard<std::mutex> lock(_send_mtx);
        if (!_send_queue.empty())
            pending = true;

        node = std::make_shared<msg_node>(msg, max_len);
        _send_queue.push(node);
    }

    if (pending)
        return;

    auto self = shared_from_this();
    boost::asio::async_write(
        _sock, boost::asio::buffer(node->data(), node->size()),
        [self](const boost::system::error_code &ec,
               std::size_t bytes_transfered) { self->handle_write(ec); });
}
