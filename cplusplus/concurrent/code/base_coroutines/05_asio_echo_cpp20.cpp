// 可选示例：需要安装 Boost.Asio 头文件；默认 make 不构建此文件。
#include <utility>
#include <boost/asio.hpp>
#include <array>
#include <charconv>
#include <csignal>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace asio = boost::asio;
using asio::ip::tcp;

void report_error(std::exception_ptr error)
{
    if (!error)
        return;
    try
    {
        std::rethrow_exception(error);
    }
    catch (const std::exception &e)
    {
        std::cerr << "coroutine completed: " << e.what() << '\n';
    }
}

asio::awaitable<void> echo_session(tcp::socket socket)
{
    std::array<char, 4096> buffer{};
    try
    {
        for (;;)
        {
            std::size_t n = co_await socket.async_read_some(
                asio::buffer(buffer), asio::use_awaitable);
            // async_write 是组合操作，负责处理部分写；buffer 活到完成。
            co_await asio::async_write(socket, asio::buffer(buffer.data(), n),
                                      asio::use_awaitable);
        }
    }
    catch (const boost::system::system_error &e)
    {
        if (e.code() != asio::error::eof)
            throw;
    }
    co_return;
}

asio::awaitable<void> listen(asio::io_context &io, unsigned short port)
{
    tcp::acceptor acceptor(io, {asio::ip::address_v4::loopback(), port});
    std::cout << "LISTEN 127.0.0.1:" << acceptor.local_endpoint().port() << std::endl;
    for (;;)
    {
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(io, echo_session(std::move(socket)), report_error);
    }
}

int main(int argc, char **argv)
try
{
    if (argc != 2)
        throw std::runtime_error("usage: asio_echo PORT");
    unsigned port = 0;
    std::string_view input(argv[1]);
    auto result = std::from_chars(input.data(), input.data() + input.size(), port);
    if (result.ec != std::errc{} || result.ptr != input.data() + input.size() || port > 65535)
        throw std::runtime_error("invalid port");

    asio::io_context io;
    asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&io](const boost::system::error_code &, int) { io.stop(); });
    asio::co_spawn(io, listen(io, static_cast<unsigned short>(port)),
                   [&io](std::exception_ptr error)
                   {
                       report_error(error);
                       if (error)
                           io.stop();
                   });
    io.run(); // 单个调用线程；可同时管理多个挂起的会话协程。
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
