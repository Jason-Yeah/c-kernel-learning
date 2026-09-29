#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cerrno>
#include <charconv>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

[[noreturn]] static void fail(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}
class unique_fd {
    int fd_ = -1;
public:
    explicit unique_fd(int fd = -1) noexcept : fd_(fd) {}
    ~unique_fd() { if (fd_ >= 0) ::close(fd_); }
    unique_fd(const unique_fd&) = delete;
    unique_fd& operator=(const unique_fd&) = delete;
    unique_fd(unique_fd&& x) noexcept : fd_(std::exchange(x.fd_, -1)) {}
    int get() const noexcept { return fd_; }
};
enum class State { reading, processing, writing, closed };
struct Connection {
    explicit Connection(int fd) : socket(fd) {}
    unique_fd socket;
    State state = State::reading;
    std::string input;
    std::string output;
    std::size_t sent = 0;
    bool peer_eof = false;
};
struct Job { std::shared_ptr<Connection> connection; std::string line; };
struct Completion { std::shared_ptr<Connection> connection; std::string response; };
class ThreadPool {
    std::mutex mutex_;
    std::condition_variable ready_;
    std::queue<Job> jobs_;
    std::vector<std::thread> workers_;
    bool stopping_ = false;
    std::mutex& done_mutex_;
    std::queue<Completion>& done_;
    int notify_fd_;
    void worker() {
        for (;;) {
            Job job;
            {
                std::unique_lock lock(mutex_);
                ready_.wait(lock, [&] { return stopping_ || !jobs_.empty(); });
                if (stopping_ && jobs_.empty()) return;
                job = std::move(jobs_.front()); jobs_.pop();
            }
            // 业务逻辑示范：把一行 ASCII 文本转换成大写。
            for (char& c : job.line) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
            Completion completed{std::move(job.connection), "OK " + job.line + "\n"};
            {
                std::lock_guard lock(done_mutex_);
                done_.push(std::move(completed));
            }
            std::uint64_t one = 1;
            while (::write(notify_fd_, &one, sizeof(one)) < 0 && errno == EINTR) {}
        }
    }
public:
    ThreadPool(std::size_t count, std::mutex& dm, std::queue<Completion>& dq, int event_fd)
        : done_mutex_(dm), done_(dq), notify_fd_(event_fd) {
        for (std::size_t i = 0; i < count; ++i) workers_.emplace_back([this] { worker(); });
    }
    ~ThreadPool() {
        { std::lock_guard lock(mutex_); stopping_ = true; }
        ready_.notify_all();
        for (auto& t : workers_) if (t.joinable()) t.join();
    }
    void submit(Job job) {
        { std::lock_guard lock(mutex_); jobs_.push(std::move(job)); }
        ready_.notify_one();
    }
};
static volatile sig_atomic_t stopping = 0;
static void on_signal(int) { stopping = 1; }
static void epoll_add(int epfd, int fd, std::uint32_t flags) {
    epoll_event ev{}; ev.events = flags; ev.data.fd = fd;
    if (::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) fail("epoll_ctl ADD");
}
static void epoll_mod(int epfd, int fd, std::uint32_t flags) {
    epoll_event ev{}; ev.events = flags; ev.data.fd = fd;
    if (::epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0) fail("epoll_ctl MOD");
}
static bool dispatch_line(const std::shared_ptr<Connection>& c, ThreadPool& pool) {
    if (c->state != State::reading) return true;
    auto end = c->input.find('\n');
    if (end == std::string::npos) {
        if (c->input.size() > 4096) { c->state = State::closed; return false; }
        return true;
    }
    if (end > 4096) { c->state = State::closed; return false; }
    std::string line = c->input.substr(0, end);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    c->input.erase(0, end + 1);
    c->state = State::processing;
    pool.submit(Job{c, std::move(line)});
    return true;
}
static std::uint32_t interest(const Connection& c) {
    std::uint32_t e = EPOLLRDHUP | EPOLLERR;
    if (c.state == State::reading && !c.peer_eof) e |= EPOLLIN;
    if (c.state == State::writing) e |= EPOLLOUT;
    return e;
}
static void close_connection(int epfd, std::unordered_map<int, std::shared_ptr<Connection>>& clients,
                             const std::shared_ptr<Connection>& c) {
    int fd = c->socket.get();
    c->state = State::closed;
    ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    auto it = clients.find(fd);
    if (it != clients.end() && it->second == c) clients.erase(it);
    std::cerr << "closed fd=" << fd << '\n';
}
static int run_server(unsigned short port, std::size_t workers) {
    unique_fd listener(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
    if (listener.get() < 0) fail("socket");
    int yes = 1;
    if (::setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) fail("setsockopt");
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(listener.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) fail("bind");
    if (::listen(listener.get(), 128) < 0) fail("listen");
    socklen_t alen = sizeof(addr);
    if (::getsockname(listener.get(), reinterpret_cast<sockaddr*>(&addr), &alen) < 0) fail("getsockname");
    unique_fd epfd(::epoll_create1(EPOLL_CLOEXEC)); if (epfd.get() < 0) fail("epoll_create1");
    unique_fd notify(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)); if (notify.get() < 0) fail("eventfd");
    epoll_add(epfd.get(), listener.get(), EPOLLIN);
    epoll_add(epfd.get(), notify.get(), EPOLLIN);
    std::mutex done_mutex;
    std::queue<Completion> done;
    ThreadPool pool(workers, done_mutex, done, notify.get());
    std::unordered_map<int, std::shared_ptr<Connection>> clients;
    std::cout << "LISTEN 127.0.0.1:" << ntohs(addr.sin_port) << " workers=" << workers << std::endl;
    std::vector<epoll_event> events(128);
    while (!stopping) {
        int n = ::epoll_wait(epfd.get(), events.data(), static_cast<int>(events.size()), -1);
        if (n < 0) { if (errno == EINTR) continue; fail("epoll_wait"); }
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == listener.get()) {
                for (;;) {
                    int clientfd = ::accept4(listener.get(), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
                    if (clientfd < 0) {
                        if (errno == EINTR) continue;
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        std::cerr << "accept4: " << std::strerror(errno) << '\n'; break;
                    }
                    auto c = std::make_shared<Connection>(clientfd);
                    clients.emplace(clientfd, c);
                    epoll_add(epfd.get(), clientfd, interest(*c));
                    std::cerr << "accepted fd=" << clientfd << '\n';
                }
            } else if (fd == notify.get()) {
                std::uint64_t value;
                while (::read(notify.get(), &value, sizeof(value)) > 0) {}
                std::queue<Completion> local;
                { std::lock_guard lock(done_mutex); std::swap(local, done); }
                while (!local.empty()) {
                    auto result = std::move(local.front()); local.pop();
                    auto c = result.connection;
                    auto it = clients.find(c->socket.get());
                    if (it == clients.end() || it->second != c || c->state == State::closed) continue;
                    c->output = std::move(result.response); c->sent = 0; c->state = State::writing;
                    epoll_mod(epfd.get(), c->socket.get(), interest(*c));
                }
            } else {
                auto it = clients.find(fd); if (it == clients.end()) continue;
                auto c = it->second;
                auto flags = events[i].events;
                bool alive = true;
                if (flags & EPOLLIN) {
                    char buf[2048];
                    for (;;) {
                        ssize_t got = ::recv(fd, buf, sizeof(buf), 0);
                        if (got > 0) { c->input.append(buf, static_cast<std::size_t>(got)); continue; }
                        if (got == 0) { c->peer_eof = true; break; }
                        if (errno == EINTR) continue;
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        alive = false; break;
                    }
                    if (alive) alive = dispatch_line(c, pool);
                }
                if (alive && (flags & EPOLLOUT) && c->state == State::writing) {
                    while (c->sent < c->output.size()) {
                        ssize_t sent = ::send(fd, c->output.data() + c->sent, c->output.size() - c->sent, MSG_NOSIGNAL);
                        if (sent > 0) { c->sent += static_cast<std::size_t>(sent); continue; }
                        if (sent < 0 && errno == EINTR) continue;
                        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
                        alive = false; break;
                    }
                    if (alive && c->sent == c->output.size()) {
                        c->output.clear(); c->sent = 0; c->state = State::reading;
                        alive = dispatch_line(c, pool);
                    }
                }
                if (flags & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) c->peer_eof = true;
                if (alive && c->peer_eof && c->state == State::reading) {
                    // 本协议以换行结束请求；EOF 时丢弃未完成行，已完成行仍可处理。
                    if (!dispatch_line(c, pool)) alive = false;
                    else if (c->state == State::reading) alive = false;
                }
                if (!alive) close_connection(epfd.get(), clients, c);
                else epoll_mod(epfd.get(), fd, interest(*c));
            }
        }
    }
    for (auto& [fd, c] : clients) { (void)fd; c->state = State::closed; }
    clients.clear();
    return 0;
}
int main(int argc, char** argv) try {
    if (argc < 2 || argc > 3) throw std::runtime_error("usage: framework_server PORT [WORKERS]");
    unsigned port = 0, count = 4;
    auto parse = [](const char* s, unsigned& out) {
        auto end = s + std::strlen(s); auto result = std::from_chars(s, end, out);
        return result.ec == std::errc{} && result.ptr == end;
    };
    if (!parse(argv[1], port) || port > 65535) throw std::runtime_error("invalid port");
    if (argc == 3 && (!parse(argv[2], count) || count == 0 || count > 128)) throw std::runtime_error("workers must be 1..128");
    struct sigaction sa{}; sa.sa_handler = on_signal; sigemptyset(&sa.sa_mask);
    if (::sigaction(SIGINT, &sa, nullptr) < 0 || ::sigaction(SIGTERM, &sa, nullptr) < 0) fail("sigaction");
    ::signal(SIGPIPE, SIG_IGN);
    return run_server(static_cast<unsigned short>(port), count);
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
