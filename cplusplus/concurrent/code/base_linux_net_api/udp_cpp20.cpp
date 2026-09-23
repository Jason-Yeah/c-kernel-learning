#include "net.hpp"
#include <array>
#include <iostream>
int main(int argc, char** argv) try {
    if (argc < 3) throw std::runtime_error("usage: udp_cpp20 server PORT | client PORT [TEXT]");
    auto fd = make_socket(SOCK_DGRAM); auto port = port_number(argv[2]);
    std::string mode = argv[1];
    if (mode == "server") {
        bind_local(fd.get(), port);
        std::cout << "PORT " << bound_port(fd.get()) << std::endl;
        std::array<char, 65536> buf{}; sockaddr_in peer{}; socklen_t len = sizeof(peer);
        ssize_t n;
        do { n = ::recvfrom(fd.get(), buf.data(), buf.size(), 0, reinterpret_cast<sockaddr*>(&peer), &len); } while (n < 0 && errno == EINTR);
        if (n < 0) fail("recvfrom");
        ssize_t sent;
        do { sent = ::sendto(fd.get(), buf.data(), static_cast<std::size_t>(n), MSG_NOSIGNAL,
                            reinterpret_cast<sockaddr*>(&peer), len); } while (sent < 0 && errno == EINTR);
        if (sent < 0) fail("sendto");
        if (sent != n) throw std::runtime_error("unexpected datagram length");
    } else if (mode == "client") {
        auto peer = loopback(port);
        check(::connect(fd.get(), reinterpret_cast<sockaddr*>(&peer), sizeof(peer)), "UDP connect");
        // UDP connect 不握手；此处设置超时，防止丢包后无限等待。
        timeval timeout{3, 0};
        check(::setsockopt(fd.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)), "timeout");
        std::string text = argc > 3 ? argv[3] : "hello UDP";
        if (text.size() > 65507) throw std::runtime_error("IPv4 UDP payload too large");
        ssize_t n;
        do { n = ::send(fd.get(), text.data(), text.size(), MSG_NOSIGNAL); } while (n < 0 && errno == EINTR);
        if (n < 0) fail("send");
        if (static_cast<std::size_t>(n) != text.size()) throw std::runtime_error("short datagram");
        std::array<char, 65536> buf{};
        do { n = ::recv(fd.get(), buf.data(), buf.size(), 0); } while (n < 0 && errno == EINTR);
        if (n < 0) fail("recv (timeout or network error)");
        std::cout.write(buf.data(), n); std::cout << '\n';
    } else throw std::runtime_error("mode must be server or client");
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
