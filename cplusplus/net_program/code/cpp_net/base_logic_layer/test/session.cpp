// session.cpp — Session 会话层实现
#include "session.hpp"
#include "logic_system.hpp"
#include "session_manager.hpp"
#include <cstring>
#include <iostream>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

Session::Session(tcp::socket socket, uint64_t id)
    : _sock(std::move(socket)), _id(id)
{
}

Session::~Session()
{
    // RAII：_sock 析构时自动关闭 socket fd
}

void Session::start()
{
    // 注册到 SessionManager（全局连接映射）
    SessionManager::get_instance().add_session(_id, shared_from_this());
    std::cout << "[session] " << _id << " started\n";
    read_header();
}

void Session::close()
{
    boost::system::error_code ec;

    // 第一步：shutdown
    // 告诉对端"我不再发了"，但内核会继续把发送缓冲区发完
    _sock.shutdown(tcp::socket::shutdown_send, ec);
    if (ec)
    {
        // shutdown 失败（如 socket 已关闭），直接 close
        _sock.close(ec);
        SessionManager::get_instance().remove_session(_id);
        return;
    }

    // 第二步：close
    _sock.close(ec);
    SessionManager::get_instance().remove_session(_id);

    std::cout << "[session] " << _id << " closed\n";
}

void Session::send(const std::vector<char> &data)
{
    // TLV 序列化：[ HEAD_LEN 字节 header ][ data ]
    uint32_t body_len = static_cast<uint32_t>(data.size());
    uint32_t net_len = htonl(body_len);

    std::array<asio::const_buffer, 2> bufs = {asio::buffer(&net_len, HEAD_LEN),
                                              asio::buffer(data)};

    boost::system::error_code ec;
    asio::write(_sock, bufs, ec);
    if (ec)
    {
        handle_error(ec);
    }
}

// ============================================================
// 内部：两步异步读
// ============================================================

void Session::read_header()
{
    auto self = shared_from_this();
    // async_read 保证读完 HEAD_LEN 字节才触发回调
    asio::async_read(
        _sock, asio::buffer(_header_buf),
        [self](boost::system::error_code ec, size_t /*n*/)
        {
            if (ec)
            {
                self->handle_error(ec);
                return;
            }

            // 解析 body 长度（网络字节序 → 主机字节序）
            uint32_t body_len;
            std::memcpy(&body_len, self->_header_buf.data(), HEAD_LEN);
            body_len = ntohl(body_len);

            if (body_len == 0 || body_len > MAX_BODY)
            {
                std::cerr << "[session] invalid body_len=" << body_len
                          << ", closing\n";
                self->close();
                return;
            }

            self->read_body(body_len);
        });
}

void Session::read_body(uint32_t body_len)
{
    auto self = shared_from_this();
    auto body = std::make_shared<std::vector<char>>(body_len);

    asio::async_read(_sock, asio::buffer(*body),
                     [self, body](boost::system::error_code ec, size_t /*n*/)
                     {
                         if (ec)
                         {
                             self->handle_error(ec);
                             return;
                         }

                         // 关键：Session 到此为止，交给逻辑层
                         self->on_message(*body);
                     });
}

void Session::on_message(const std::vector<char> &body)
{
    // 投递到逻辑系统（跨线程消息队列）
    LogicSystem::get_instance().post_message(_id, 0, body);

    // 继续读下一个消息（形成循环）
    read_header();
}

void Session::handle_error(const boost::system::error_code &ec)
{
    if (ec == asio::error::eof)
    {
        std::cout << "[session] " << _id << " peer closed\n";
    }
    else
    {
        std::cerr << "[session] " << _id << " error: " << ec.message() << "\n";
    }
    close();
}
