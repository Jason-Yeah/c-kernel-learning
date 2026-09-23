#include "net.hpp"
#include <netdb.h>
#include <poll.h>
#include <sys/uio.h>
#include <array>
#include <algorithm>
#include <iostream>
#include <memory>

void resolve(const char* host, const char* service) {
    addrinfo hints{}; hints.ai_family = AF_UNSPEC; hints.ai_socktype = SOCK_STREAM;
    addrinfo* raw = nullptr;
    int rc = ::getaddrinfo(host, service, &hints, &raw);
    if (rc != 0) throw std::runtime_error(gai_strerror(rc));
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> list(raw, freeaddrinfo);
    for (auto p = list.get(); p; p = p->ai_next) {
        char h[NI_MAXHOST]{}, s[NI_MAXSERV]{};
        rc = ::getnameinfo(p->ai_addr, p->ai_addrlen, h, sizeof(h), s, sizeof(s), NI_NUMERICHOST | NI_NUMERICSERV);
        if (rc != 0) throw std::runtime_error(gai_strerror(rc));
        std::cout << (p->ai_family == AF_INET6 ? "IPv6 " : "IPv4 ") << h << ':' << s << '\n';
    }
    // 老接口常返回静态存储；立即使用，绝不 free，也不跨下一次查询保存指针。
    if (auto entry = ::getservbyname("http", "tcp"))
        std::cout << "legacy service http/tcp = " << ntohs(static_cast<uint16_t>(entry->s_port)) << '\n';
}
void options() {
    auto fd = make_socket(SOCK_STREAM);
    int desired = 32768;
    check(::setsockopt(fd.get(), SOL_SOCKET, SO_SNDBUF, &desired, sizeof(desired)), "SO_SNDBUF");
    for (auto option : {SO_REUSEADDR, SO_SNDBUF, SO_RCVBUF, SO_RCVLOWAT, SO_SNDLOWAT}) {
        int value{}; socklen_t size = sizeof(value);
        check(::getsockopt(fd.get(), SOL_SOCKET, option, &value, &size), "getsockopt");
        std::cout << "option " << option << " = " << value << '\n';
    }
    linger value{}; socklen_t len = sizeof(value);
    check(::getsockopt(fd.get(), SOL_SOCKET, SO_LINGER, &value, &len), "SO_LINGER");
    std::cout << "linger enabled=" << value.l_onoff << " seconds=" << value.l_linger << '\n';
}
void message() {
    auto receiver = make_socket(SOCK_DGRAM); auto sender = make_socket(SOCK_DGRAM);
    bind_local(receiver.get(), 0); auto dest = loopback(bound_port(receiver.get()));
    char first[] = "HEAD:"; char second[] = "body";
    iovec parts[] = {{first, 5}, {second, 4}};
    msghdr out{}; out.msg_name = &dest; out.msg_namelen = sizeof(dest);
    out.msg_iov = parts; out.msg_iovlen = 2;
    ssize_t n;
    do { n = ::sendmsg(sender.get(), &out, MSG_NOSIGNAL); } while (n < 0 && errno == EINTR);
    if (n < 0) fail("sendmsg");
    if (n != 9) throw std::runtime_error("unexpected sendmsg length");
    timeval timeout{3, 0};
    check(::setsockopt(receiver.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)), "timeout");
    // 故意只接收 7 字节，观察一个 9 字节数据报被截断。
    std::array<char, 3> a{}; std::array<char, 4> b{};
    iovec target[] = {{a.data(), a.size()}, {b.data(), b.size()}};
    msghdr in{}; in.msg_iov = target; in.msg_iovlen = 2;
    do { n = ::recvmsg(receiver.get(), &in, 0); } while (n < 0 && errno == EINTR);
    if (n < 0) fail("recvmsg");
    std::cout << "copied=" << n << " truncated=" << ((in.msg_flags & MSG_TRUNC) != 0) << " data=";
    auto used_a = std::min(static_cast<std::size_t>(n), a.size());
    std::cout.write(a.data(), static_cast<std::streamsize>(used_a));
    std::cout.write(b.data(), n - static_cast<ssize_t>(used_a)); std::cout << '\n';
}
void oob() {
    auto listener = make_socket(SOCK_STREAM); bind_local(listener.get(), 0);
    check(::listen(listener.get(), 1), "listen");
    auto sender = make_socket(SOCK_STREAM); auto addr = loopback(bound_port(listener.get()));
    check(::connect(sender.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), "connect");
    int raw;
    do { raw = ::accept4(listener.get(), nullptr, nullptr, SOCK_CLOEXEC); } while (raw < 0 && errno == EINTR);
    check(raw, "accept4"); unique_fd receiver(raw);
    int yes = 1;
    check(::setsockopt(receiver.get(), SOL_SOCKET, SO_OOBINLINE, &yes, sizeof(yes)), "SO_OOBINLINE");
    send_all(sender.get(), std::span<const char>("abc", 3));
    ssize_t n;
    do { n = ::send(sender.get(), "!", 1, MSG_OOB | MSG_NOSIGNAL); } while (n < 0 && errno == EINTR);
    if (n != 1) { if (n < 0) fail("MSG_OOB"); throw std::runtime_error("short OOB send"); }
    send_all(sender.get(), std::span<const char>("xyz", 3));
    check(::shutdown(sender.get(), SHUT_WR), "shutdown");
    for (;;) {
        pollfd event{receiver.get(), POLLIN, 0};
        int ready;
        do { ready = ::poll(&event, 1, 3000); } while (ready < 0 && errno == EINTR);
        check(ready, "poll"); if (ready == 0) throw std::runtime_error("OOB timeout");
        int mark = ::sockatmark(receiver.get()); check(mark, "sockatmark");
        char c{};
        do { n = ::recv(receiver.get(), &c, 1, 0); } while (n < 0 && errno == EINTR);
        if (n < 0) fail("recv");
        if (n == 0) break;
        std::cout << "mark=" << mark << " byte=" << c << '\n';
    }
}
int main(int argc, char** argv) try {
    if (argc < 2) throw std::runtime_error("usage: api_lab resolve [HOST SERVICE] | options | msg | oob");
    std::string mode = argv[1];
    if (mode == "resolve") resolve(argc > 2 ? argv[2] : "localhost", argc > 3 ? argv[3] : "80");
    else if (mode == "options") options();
    else if (mode == "msg") message();
    else if (mode == "oob") oob();
    else throw std::runtime_error("unknown mode");
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
