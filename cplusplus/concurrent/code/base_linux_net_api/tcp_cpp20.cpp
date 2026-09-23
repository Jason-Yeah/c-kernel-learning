#include "net.hpp"
#include <iostream>
int main(int argc, char** argv) try {
    if (argc < 3) throw std::runtime_error("usage: tcp_cpp20 server PORT | client PORT [TEXT]");
    std::string mode = argv[1]; auto port = port_number(argv[2]);
    if (mode == "server") {
        auto listener = make_socket(SOCK_STREAM);
        int yes = 1;
        check(::setsockopt(listener.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)), "setsockopt");
        bind_local(listener.get(), port);
        check(::listen(listener.get(), 16), "listen");
        std::cout << "PORT " << bound_port(listener.get()) << std::endl;
        int raw;
        do { raw = ::accept4(listener.get(), nullptr, nullptr, SOCK_CLOEXEC); } while (raw < 0 && errno == EINTR);
        check(raw, "accept4"); unique_fd peer(raw);
        sockaddr_in remote{}; socklen_t size = sizeof(remote);
        check(::getpeername(peer.get(), reinterpret_cast<sockaddr*>(&remote), &size), "getpeername");
        char ip[INET_ADDRSTRLEN]{};
        if (!::inet_ntop(AF_INET, &remote.sin_addr, ip, sizeof(ip))) fail("inet_ntop");
        std::cerr << "peer " << ip << ':' << ntohs(remote.sin_port) << '\n';
        std::vector<char> body;
        while (recv_frame(peer.get(), body)) send_frame(peer.get(), body);
        check(::shutdown(peer.get(), SHUT_WR), "shutdown");
    } else if (mode == "client") {
        auto fd = make_socket(SOCK_STREAM); auto addr = loopback(port);
        check(::connect(fd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)), "connect");
        std::string text = argc > 3 ? argv[3] : "hello C++20";
        send_frame(fd.get(), text);
        check(::shutdown(fd.get(), SHUT_WR), "shutdown");
        std::vector<char> reply;
        if (!recv_frame(fd.get(), reply)) throw std::runtime_error("missing response");
        std::cout.write(reply.data(), static_cast<std::streamsize>(reply.size()));
        std::cout << '\n';
        if (recv_frame(fd.get(), reply)) throw std::runtime_error("unexpected extra frame");
    } else throw std::runtime_error("mode must be server or client");
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
