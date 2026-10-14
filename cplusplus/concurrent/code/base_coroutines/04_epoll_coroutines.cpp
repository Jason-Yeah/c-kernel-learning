#include "task.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

[[noreturn]] static void fail(const char *what)
{
    throw std::system_error(errno, std::generic_category(), what);
}

class UniqueFd
{
    int fd_ = -1;

public:
    explicit UniqueFd(int fd = -1) noexcept : fd_(fd) {}
    ~UniqueFd() { if (fd_ >= 0) ::close(fd_); }
    UniqueFd(const UniqueFd &) = delete;
    UniqueFd &operator=(const UniqueFd &) = delete;
    UniqueFd(UniqueFd &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    int get() const noexcept { return fd_; }
};

class EventLoop
{
    struct Registration
    {
        int fd;
        Task::Handle handle; // 借用；Task 对象才拥有协程帧。
    };

    UniqueFd epoll_;
    std::uint64_t next_token_ = 1;
    std::unordered_map<std::uint64_t, Registration> waiting_;

    std::uint64_t watch(int fd, std::uint32_t events, Task::Handle h)
    {
        auto token = next_token_++;
        waiting_.emplace(token, Registration{fd, h});
        epoll_event event{};
        event.events = events;
        event.data.u64 = token; // 旧事件不会误认为是复用 fd 的新等待者。
        if (::epoll_ctl(epoll_.get(), EPOLL_CTL_ADD, fd, &event) < 0)
        {
            int saved = errno;
            waiting_.erase(token);
            errno = saved;
            fail("epoll_ctl ADD (only one waiter per fd is supported)");
        }
        return token;
    }

    void cancel(std::uint64_t token) noexcept
    {
        auto it = waiting_.find(token);
        if (it == waiting_.end())
            return;
        ::epoll_ctl(epoll_.get(), EPOLL_CTL_DEL, it->second.fd, nullptr);
        waiting_.erase(it);
    }

public:
    EventLoop() : epoll_(::epoll_create1(EPOLL_CLOEXEC))
    {
        if (epoll_.get() < 0)
            fail("epoll_create1");
    }

    class Waiter
    {
        EventLoop &loop_;
        int fd_;
        std::uint32_t events_;
        std::uint64_t token_ = 0;

    public:
        Waiter(EventLoop &loop, int fd, std::uint32_t events)
            : loop_(loop), fd_(fd), events_(events) {}
        Waiter(const Waiter &) = delete;
        Waiter &operator=(const Waiter &) = delete;
        ~Waiter() { loop_.cancel(token_); }

        bool await_ready() const noexcept { return false; }
        void await_suspend(Task::Handle h) { token_ = loop_.watch(fd_, events_, h); }
        void await_resume() const noexcept {}
    };

    Waiter readable(int fd) { return Waiter{*this, fd, EPOLLIN}; }
    Waiter writable(int fd) { return Waiter{*this, fd, EPOLLOUT}; }
    std::size_t pending() const noexcept { return waiting_.size(); }

    void run()
    {
        std::array<epoll_event, 16> events{};
        while (!waiting_.empty())
        {
            int count = ::epoll_wait(epoll_.get(), events.data(),
                                     static_cast<int>(events.size()), 5000);
            if (count < 0)
            {
                if (errno == EINTR)
                    continue;
                fail("epoll_wait");
            }
            if (count == 0)
                throw std::runtime_error("demo made no I/O progress for 5 seconds");

            for (int i = 0; i < count; ++i)
            {
                auto token = events[i].data.u64;
                auto it = waiting_.find(token);
                if (it == waiting_.end())
                    continue; // 等待已取消，这次批次里的旧事件被丢弃。

                auto h = it->second.handle;
                cancel(token); // 先注销，再恢复；协程可立即为同一 fd 注册新等待。
                h.resume();
                if (h.done() && h.promise().error)
                    std::rethrow_exception(h.promise().error);
            }
        }
    }
};

static std::array<UniqueFd, 2> socket_pair()
{
    int fds[2];
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, fds) < 0)
        fail("socketpair");
    return {UniqueFd{fds[0]}, UniqueFd{fds[1]}};
}

Task delayed_sender(EventLoop &loop, UniqueFd socket, std::string name, int milliseconds)
{
    UniqueFd timer(::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC));
    if (timer.get() < 0)
        fail("timerfd_create");
    itimerspec spec{};
    spec.it_value.tv_sec = milliseconds / 1000;
    spec.it_value.tv_nsec = (milliseconds % 1000) * 1000000L;
    if (::timerfd_settime(timer.get(), 0, &spec, nullptr) < 0)
        fail("timerfd_settime");

    std::cout << "sender " << name << ": waiting for timer\n";
    co_await loop.readable(timer.get()); // 让出执行权，没有 sleep 阻塞线程。
    std::uint64_t expirations = 0;
    if (::read(timer.get(), &expirations, sizeof(expirations)) < 0)
        fail("read timerfd");

    std::string message = "hello " + name + "\n";
    std::size_t sent = 0;
    while (sent < message.size())
    {
        auto n = ::send(socket.get(), message.data() + sent,
                        message.size() - sent, MSG_NOSIGNAL);
        if (n > 0)
        {
            sent += static_cast<std::size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            co_await loop.writable(socket.get());
            continue; // 就绪通知后重新尝试 send。
        }
        if (n < 0)
            fail("send");
        throw std::runtime_error("send made no progress");
    }
    if (::shutdown(socket.get(), SHUT_WR) < 0)
        fail("shutdown");
    std::cout << "sender " << name << ": finished\n";
    co_return;
}

Task receiver(EventLoop &loop, UniqueFd socket, std::string name)
{
    std::string received;
    std::array<char, 64> buffer{};
    for (;;)
    {
        auto n = ::recv(socket.get(), buffer.data(), buffer.size(), 0);
        if (n > 0)
        {
            received.append(buffer.data(), static_cast<std::size_t>(n));
            continue;
        }
        if (n == 0)
            break;
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            co_await loop.readable(socket.get());
            continue;
        }
        fail("recv");
    }
    std::cout << "receiver " << name << ": " << received;
    co_return;
}

Task unused_wait(EventLoop &loop, int fd)
{
    co_await loop.readable(fd);
    co_return;
}

int main(int argc, char **argv)
try
{
    EventLoop loop; // 必须比任务所有者活得更久。
    if (argc == 2 && std::string(argv[1]) == "--cancel-demo")
    {
        auto pair = socket_pair();
        {
            auto task = unused_wait(loop, pair[0].get());
            task.resume();
            std::cout << "before task destruction: pending=" << loop.pending() << '\n';
        } // destroy() 销毁挂起点的 Waiter；它撤销 epoll 注册。
        std::cout << "after task destruction: pending=" << loop.pending() << '\n';
        return loop.pending() == 0 ? 0 : 1;
    }
    if (argc != 1)
        throw std::runtime_error("usage: epoll_coroutines [--cancel-demo]");

    auto a = socket_pair();
    auto b = socket_pair();
    std::vector<Task> tasks;
    tasks.emplace_back(receiver(loop, std::move(a[0]), "A"));
    tasks.emplace_back(receiver(loop, std::move(b[0]), "B"));
    tasks.emplace_back(delayed_sender(loop, std::move(a[1]), "A", 80));
    tasks.emplace_back(delayed_sender(loop, std::move(b[1]), "B", 30));
    for (auto &task : tasks)
        task.resume(); // 只启动一次；之后由 epoll 事件循环恢复。
    loop.run();
    for (const auto &task : tasks)
    {
        task.rethrow_if_failed();
        if (!task.done())
            throw std::runtime_error("no registered I/O, but a task is unfinished");
    }
    std::cout << "all four tasks completed on one thread\n";
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
