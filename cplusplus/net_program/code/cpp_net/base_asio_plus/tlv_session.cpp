#include "tlv_session.hpp"
#include "server.hpp"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>

using namespace std;

// send
msg_node::msg_node(char *msg, short max_len)
    : _total_len(max_len + HEAD_LENGTH), _cur_len(0)
{
    _data = new char[_total_len + 1]();
    // len + data
    memcpy(_data, &max_len, HEAD_LENGTH);
    memcpy(_data + HEAD_LENGTH, msg, max_len);
    _data[_total_len] = '\0';
}
// recv
msg_node::msg_node(short max_len) { _data = new char[max_len + 1](); }

msg_node::~msg_node() { delete[] _data; }

void msg_node::clear()
{
    memset(_data, 0, _total_len);
    _cur_len = 0;
}

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
void session::handle_read(const boost::system::error_code &ec,
                          std::size_t bytes_transferred)
{
    if (!ec)
    {
        int cp_len = 0;
        while (bytes_transferred > 0)
        {
            if (!_is_head_parse)
            {
                if (bytes_transferred + _recv_head_node->_cur_len <
                    HEAD_LENGTH) // 收到数据不足头部大小
                {
                    memcpy(_recv_head_node->_data + _recv_head_node->_cur_len,
                           _data + cp_len, bytes_transferred);
                    _recv_head_node->_cur_len += bytes_transferred;
                    memset(_data, 0, MSG_MAX_LENGTH);

                    auto self = shared_from_this();
                    _sock.async_read_some(
                        boost::asio::buffer(_data, MSG_MAX_LENGTH),
                        [self](const boost::system::error_code &ec,
                               std::size_t bytes_transferred)
                        { self->handle_read(ec, bytes_transferred); });
                    return;
                }

                // 头部剩余未复制的长度
                int head_remain = HEAD_LENGTH - _recv_head_node->_cur_len;
                memcpy(_recv_head_node->_data + _recv_head_node->_cur_len,
                       _data + cp_len, head_remain);
                cp_len += head_remain;
                bytes_transferred -= head_remain;

                short data_len = 0;
                memcpy(&data_len, _recv_head_node->_data, HEAD_LENGTH);
                std::cout << "data_len is " << data_len << std::endl;

                if (data_len > MSG_MAX_LENGTH)
                {
                    std::cout << "invalid data length is " << data_len
                              << std::endl;
                    _server->clear_session(_uuid);
                    return;
                }
                _recv_msg_node = std::make_shared<msg_node>(data_len);

                if (bytes_transferred < data_len)
                {
                    memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len,
                           _data + cp_len, bytes_transferred);
                    _recv_msg_node->_cur_len += bytes_transferred;
                    memset(_data, 0, MSG_MAX_LENGTH);
                    _sock.async_read_some(
                        boost::asio::buffer(_data, MSG_MAX_LENGTH),
                        [self = shared_from_this()](
                            const boost::system::error_code &ec,
                            std::size_t bytes_transferred)
                        { self->handle_read(ec, bytes_transferred); });

                    _is_head_parse = true;
                    return;
                }

                // 可能粘包了先拷贝处理
                memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len,
                       _data + cp_len, data_len);
                _recv_msg_node->_cur_len += data_len;
                cp_len += data_len;
                bytes_transferred -= data_len;
                _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
                cout << "receive data is " << _recv_msg_node->_data << endl;
                // 此处可以调用Send发送测试
                send(_recv_msg_node->_data, _recv_msg_node->_total_len);
                // 继续轮询剩余未处理数据
                _is_head_parse = false;
                _recv_head_node->clear();
                if (bytes_transferred <= 0)
                {
                    ::memset(_data, 0, MAX_LENGTH);
                    _sock.async_read_some(
                        boost::asio::buffer(_data, MAX_LENGTH),
                        std::bind(&session::handle_read, this,
                                  std::placeholders::_1,
                                  std::placeholders::_2));
                    return;
                }
                continue;
            }

            // 已经处理完头部，处理上次未接受完的消息数据
            // 接收的数据仍不足剩余未处理的
            int remain_msg =
                _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
            if (bytes_transferred < remain_msg)
            {
                memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len,
                       _data + cp_len, bytes_transferred);
                _recv_msg_node->_cur_len += bytes_transferred;
                ::memset(_data, 0, MAX_LENGTH);
                _sock.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                      std::bind(&session::handle_read, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
                return;
            }
            memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len,
                   _data + cp_len, remain_msg);
            _recv_msg_node->_cur_len += remain_msg;
            bytes_transferred -= remain_msg;
            cp_len += remain_msg;
            _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
            cout << "receive data is " << _recv_msg_node->_data << endl;
            // 此处可以调用Send发送测试
            send(_recv_msg_node->_data, _recv_msg_node->_total_len);
            // 继续轮询剩余未处理数据
            _is_head_parse = false;
            _recv_head_node->clear();
            if (bytes_transferred <= 0)
            {
                ::memset(_data, 0, MAX_LENGTH);
                _sock.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                      std::bind(&session::handle_read, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
                return;
            }
            continue;
        }
    }
    else
    {
        std::cout << "READ ERROR" << std::endl;
        // close();
        _server->clear_session(_uuid);
    }
}

// ECHO SERVICE
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
        _sock, boost::asio::buffer(node->_data, node->_total_len),
        [self](const boost::system::error_code &ec,
               std::size_t bytes_transfered) { self->handle_write(ec); });
}
