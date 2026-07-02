#include "session.hpp"

// Sender
msg_node::msg_node(const char *msg, int total) : _total_len(total), _cur_len(0)
{
    _msg = new char[total];
    memcpy(_msg, msg, total);
}

// Receiver
msg_node::msg_node(int total) : _total_len(total), _cur_len(0)
{
    _msg = new char[total];
}

msg_node::~msg_node() { delete[] _msg; }

session::session(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
    : _sock(sock), _send_pending(false), _recv_pending(false)
{
}

void session::connect(const boost::asio::ip::tcp::endpoint &ep)
{
    _sock->connect(ep);
}

void session::write_callback_err(const boost::system::error_code &ec,
                                 std::size_t bytes_transferred,
                                 std::shared_ptr<msg_node> send_node)
{
    if (ec)
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return;
    }

    send_node->_cur_len += bytes_transferred;

    if (send_node->_cur_len < send_node->_total_len)
    {
        auto self = shared_from_this();
        _sock->async_write_some(
            boost::asio::buffer(_send_node->_msg + _send_node->_cur_len,
                                _send_node->_total_len - _send_node->_cur_len),
            [self, send_node](const boost::system::error_code &ec,
                              std::size_t bytes_transferred)
            { self->write_callback_err(ec, bytes_transferred, send_node); });
    }
}

void session::write2socket_err(const std::string &buf)
{
    auto send_node = std::make_shared<msg_node>(buf.data(), buf.size());
    _send_node = send_node;

    auto self = shared_from_this();
    /*
    _sock->async_write_some(
        boost::asio::buffer(_send_node->_msg, _send_node->_total_len),
        std::bind(&session::write_callback_err, shared_from_this(),
                  std::placeholders::_1, std::placeholders::_2, _send_node));
    */

    _sock->async_write_some(
        boost::asio::buffer(send_node->_msg, send_node->_total_len),
        [self, send_node](const boost::system::error_code &ec,
                          std::size_t bytes_transferred)
        { self->write_callback_err(ec, bytes_transferred, send_node); });
}

void session::write_callback(const boost::system::error_code &ec,
                             std::size_t bytes_transferred)
{
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return;
    }

    auto &send_data = _send_queue.front();
    send_data->_cur_len += bytes_transferred;
    auto self = shared_from_this();

    if (send_data->_cur_len < send_data->_total_len)
    {
        _sock->async_write_some(
            boost::asio::buffer(send_data->_msg + send_data->_cur_len,
                                send_data->_total_len - send_data->_cur_len),
            [self](const boost::system::error_code &ec,
                   std::size_t bytes_transferred)
            { self->write_callback(ec, bytes_transferred); });
        return;
    }

    _send_queue.pop();

    if (_send_queue.empty())
        _send_pending = false;
    else
    {
        auto &send_data = _send_queue.front();
        _sock->async_write_some(
            boost::asio::buffer(send_data->_msg + send_data->_cur_len,
                                send_data->_total_len - send_data->_cur_len),
            [self](const boost::system::error_code &ec,
                   std::size_t bytes_transferred)
            { self->write_callback(ec, bytes_transferred); });
    }
}

void session::write2socket(const std::string &buf)
{
    auto node = std::make_shared<msg_node>(buf.data(), buf.size());
    _send_queue.emplace(std::move(node));

    if (_send_pending)
        return; // queue still have data. waiting...

    _send_pending = true;
    auto self = shared_from_this();

    auto &front_node = _send_queue.front();

    _sock->async_write_some(
        boost::asio::buffer(front_node->_msg, front_node->_total_len),
        [self](const boost::system::error_code &ec,
               std::size_t bytes_transferred)
        { self->write_callback(ec, bytes_transferred); });
}

void session::write_all_callback(const boost::system::error_code &ec,
                                 std::size_t bytes_transferred)
{
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return;
    }

    _send_queue.pop();
    if (_send_queue.empty())
        _send_pending = false;
    if (!_send_queue.empty())
    {
        auto &send_data = _send_queue.front();
        auto self = shared_from_this();
        _sock->async_write_some(
            boost::asio::buffer(send_data->_msg + send_data->_cur_len,
                                send_data->_total_len - send_data->_cur_len),
            [self](const boost::system::error_code &ec,
                   std::size_t bytes_transferred)
            { self->write_callback(ec, bytes_transferred); });
    }
}

void session::write_all_to_socket(const std::string &buf)
{
    auto node = std::make_shared<msg_node>(buf.data(), buf.size());
    _send_queue.emplace(std::move(node));

    if (_send_pending)
        return;

    auto self = shared_from_this();
    this->_sock->async_send(boost::asio::buffer(buf),
                            [self](const boost::system::error_code &ec,
                                   std::size_t bytes_transferred) {
                                self->write_all_callback(ec, bytes_transferred);
                            });
    _send_pending = true;
}

void session::read_from_socket()
{
    if (_recv_pending)
        return;

    // Alloc the receive buffer...
    _recv_node = std::make_shared<msg_node>(RECVSIZE);
    auto self = shared_from_this();
    _sock->async_read_some(
        boost::asio::buffer(_recv_node->_msg, _recv_node->_total_len),
        [self](const boost::system::error_code &ec,
               std::size_t bytes_transferred)
        { self->read_call_back(ec, bytes_transferred); });

    _recv_pending = true;
}

void session::read_call_back(const boost::system::error_code &ec,
                             std::size_t bytes_transferred)
{
    _recv_node->_cur_len += bytes_transferred;
    if (_recv_node->_cur_len < _recv_node->_total_len)
    {
        auto self = shared_from_this();
        _sock->async_read_some(
            boost::asio::buffer(_recv_node->_msg + _recv_node->_cur_len,
                                _recv_node->_total_len - _recv_node->_cur_len),
            [self](const boost::system::error_code &ec,
                   std::size_t bytes_transferred)
            { self->write_callback(ec, bytes_transferred); });
        return;
    }

    _recv_pending = false;
    _recv_node = nullptr;
}

void session::read_all_from_socket()
{
    if (_recv_pending)
        return;

    _recv_node = std::make_shared<msg_node>(RECVSIZE);
    auto self = shared_from_this();
    _sock->async_receive(
        boost::asio::buffer(_recv_node->_msg, _recv_node->_total_len),
        [self](const boost::system::error_code &ec,
               std::size_t bytes_transferred)
        { self->read_call_back(ec, bytes_transferred); });

    _recv_pending = true;
}

void session::read_all_callback(const boost::system::error_code &ec,
                                std::size_t bytes_transferred)
{
    _recv_node->_cur_len += bytes_transferred;
    _recv_node = nullptr;
    _recv_pending = false;
}
