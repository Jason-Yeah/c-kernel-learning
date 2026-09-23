#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <string>
#include <vector>

inline void fail(const char* what) { throw std::system_error(errno, std::generic_category(), what); }
inline void check(int n, const char* what) { if (n == -1) fail(what); }
// 独占 fd：禁止复制，移动时把旧对象置为 -1，析构只关闭一次。
class unique_fd {
    int fd_ = -1;
public:
    explicit unique_fd(int fd = -1) noexcept : fd_(fd) {}
    ~unique_fd() { if (fd_ >= 0) ::close(fd_); }
    unique_fd(const unique_fd&) = delete;
    unique_fd& operator=(const unique_fd&) = delete;
    unique_fd(unique_fd&& other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    unique_fd& operator=(unique_fd&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }
    int get() const noexcept { return fd_; }
};
inline unique_fd make_socket(int type) {
    int fd = ::socket(AF_INET, type | SOCK_CLOEXEC, 0);
    check(fd, "socket");
    return unique_fd(fd);
}
inline sockaddr_in loopback(unsigned short port) {
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(port);
    if (::inet_pton(AF_INET, "127.0.0.1", &a.sin_addr) != 1)
        throw std::runtime_error("inet_pton");
    return a;
}
inline unsigned short port_number(const char* text) {
    std::string s(text);
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("port must be 0..65535");
    auto n = std::stoul(s);
    if (n > 65535) throw std::runtime_error("port must be 0..65535");
    return static_cast<unsigned short>(n);
}
inline unsigned short bound_port(int fd) {
    sockaddr_in a{}; socklen_t len = sizeof(a);
    check(::getsockname(fd, reinterpret_cast<sockaddr*>(&a), &len), "getsockname");
    return ntohs(a.sin_port);
}
inline void bind_local(int fd, unsigned short port) {
    auto a = loopback(port);
    check(::bind(fd, reinterpret_cast<const sockaddr*>(&a), sizeof(a)), "bind");
}
// 仅用于阻塞 TCP。非阻塞 fd 需要保存进度、等待就绪后再继续。
inline void send_all(int fd, std::span<const char> bytes) {
    while (!bytes.empty()) {
        auto n = ::send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL);
        if (n < 0) { if (errno == EINTR) continue; fail("send"); }
        if (n == 0) throw std::runtime_error("send made no progress");
        bytes = bytes.subspan(static_cast<std::size_t>(n));
    }
}
inline bool recv_exact(int fd, std::span<char> bytes) {
    std::size_t done = 0;
    while (done < bytes.size()) {
        auto n = ::recv(fd, bytes.data() + done, bytes.size() - done, 0);
        if (n < 0) { if (errno == EINTR) continue; fail("recv"); }
        if (n == 0) {
            if (done == 0) return false;
            throw std::runtime_error("EOF in the middle of a field");
        }
        done += static_cast<std::size_t>(n);
    }
    return true;
}
constexpr std::size_t max_frame = 1024 * 1024;
inline void send_frame(int fd, std::span<const char> body) {
    if (body.size() > max_frame) throw std::runtime_error("frame too large");
    auto len = htonl(static_cast<std::uint32_t>(body.size()));
    send_all(fd, {reinterpret_cast<const char*>(&len), sizeof(len)});
    send_all(fd, body);
}
inline bool recv_frame(int fd, std::vector<char>& body) {
    std::uint32_t len{};
    if (!recv_exact(fd, {reinterpret_cast<char*>(&len), sizeof(len)})) return false;
    auto size = ntohl(len);
    if (size > max_frame) throw std::runtime_error("frame too large");
    body.resize(size);
    if (!recv_exact(fd, body)) throw std::runtime_error("EOF before body");
    return true;
}
