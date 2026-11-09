#include <boost/asio.hpp>
#include <chrono>
#include <exception>
#include <iostream>
#include <string>

namespace asio = boost::asio;

asio::awaitable<void> delayed_print(std::string name, int milliseconds)
{
    auto executor = co_await asio::this_coro::executor;
    asio::steady_timer timer(executor);
    timer.expires_after(std::chrono::milliseconds(milliseconds));

    std::cout << name << ": waiting\n";
    co_await timer.async_wait(asio::use_awaitable);
    std::cout << name << ": finished\n";
    co_return;
}

void on_complete(std::exception_ptr error)
{
    if (!error)
        return;
    try
    {
        std::rethrow_exception(error);
    }
    catch (const std::exception &e)
    {
        std::cerr << "task failed: " << e.what() << '\n';
    }
}

int main()
{
    asio::io_context io;
    asio::co_spawn(io, delayed_print("A", 80), on_complete);
    asio::co_spawn(io, delayed_print("B", 30), on_complete);
    io.run();
}