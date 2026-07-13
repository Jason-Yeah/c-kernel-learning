#include "session.hpp"
#include "session_manager.hpp"
#include "logic_system.hpp"
#include <iostream>
#include <cstring>


using boost::asio::ip::tcp;
namespace asio = boost::asio;

session::session(tcp::socket socket, uint64_t id)
    : _sock(std::move(socket)), _id(id)
{
}

session::~session()
{
    // RAII：_sock 析构时自动关闭 socket fd
}



