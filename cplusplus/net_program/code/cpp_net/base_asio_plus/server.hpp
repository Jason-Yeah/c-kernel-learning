#pragma once

#include "session.hpp"
#include <boost/asio.hpp> 
#include <map>
#include <memory>
#include <string>

class server : public std::enable_shared_from_this<server>
{
public:
    server(boost::asio::io_context &ec, unsigned short port);

    // 必须在 make_shared 完成后单独调用，不能在构造函数中调用
    void run();

    void clear_session(std::string uuid);

private:
    void start_accept();

    void handle_accept(std::shared_ptr<session> new_session,
                       const boost::system::error_code &ec);

private:
    boost::asio::io_context &_ioc;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<session>> _sessions;
};