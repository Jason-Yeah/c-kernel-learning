#include "server.hpp"
#include <iostream>

server::server(boost::asio::io_context &ioc, unsigned short port)
    : _ioc(ioc), _acceptor(ioc, boost::asio::ip::tcp::endpoint(
                                    boost::asio::ip::tcp::v4(), port))
{
    std::cout << "Server start success, on port: " << port << std::endl;
}

void server::run() { start_accept(); }

void server::clear_session(std::string uuid) { _sessions.erase(uuid); }

void server::start_accept()
{
    // auto* n_session = new session(_ioc); // 有内存泄露风险
    auto new_session = std::make_shared<session>(_ioc, this);
    auto self = shared_from_this();

    _acceptor.async_accept(
        new_session->get_socket(),
        [self, new_session](const boost::system::error_code &ec)
        { self->handle_accept(new_session, ec); });
}

void server::handle_accept(std::shared_ptr<session> new_session,
                           const boost::system::error_code &ec)
{
    if (!ec)
    {
        new_session->start();
        // _sessions.insert(std::make_pair(new_session->get_uuid(),
        // new_session));
        _sessions.insert(
            {new_session->get_uuid(),
             new_session}); // ref_count = 3(lambda + parameter + map)
    }
    else
    {
        std::cerr << "Accept error: " << ec.message() << std::endl;
        // new_session is `shared_ptr`. It is automatically released when it
        // goes out of scope, with no memory leaks.
    }

    //  无论本次 accept 成功还是失败，都继续接受下一个连接
    // （除非是致命错误如 acceptor 被主动关闭，可加判断跳过）
    if (ec != boost::asio::error::operation_aborted)
        start_accept();
}