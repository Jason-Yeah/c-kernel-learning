
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

constexpr int MAX_CLIENTS = 64;
constexpr std::size_t BUFFER_SIZE = 2048;

// 1. RAII: 自动管理文件描述符
class Socket
{
public:
    explicit Socket(int fd = -1) : fd_(fd) {}

    ~Socket()
    {
        if (fd_ >= 0)
            ::close(fd_);
    }

    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    Socket(Socket &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}

    Socket &operator=(Socket &&other) noexcept
    {
        if (this != &other)
        {
            if (fd_ >= 0)
                ::close(fd_);
            fd_ = std::exchange(other.fd_, -1);
        }
        return *this;
    }

    int get() const { return fd_; }

    void reset(int fd = -1)
    {
        if (fd_ >= 0)
            ::close(fd_);
        fd_ = fd;
    }

private:
    int fd_;
};

// 2. 发送全部字节（简化的阻塞式发送）
bool send_all(int fd, std::string_view msg)
{
    std::size_t sent = 0;

    while (sent < msg.size())
    {
        ssize_t n =
            ::send(fd, msg.data() + sent, msg.size() - sent, MSG_NOSIGNAL);

        if (n > 0)
        {
            sent += static_cast<std::size_t>(n);
        }
        else if (n < 0 && errno == EINTR)
        {
            continue;
        }
        else
        {
            return false;
        }
    }

    return true;
}

// 3. 服务端
class ChatServer
{
public:
    explicit ChatServer(unsigned short port) : port_(port) {}

    bool start()
    {
        listen_socket_.reset(::socket(AF_INET, SOCK_STREAM, 0));

        if (listen_socket_.get() < 0)
        {
            perror("socket");
            return false;
        }

        int yes = 1;
        ::setsockopt(listen_socket_.get(), SOL_SOCKET, SO_REUSEADDR, &yes,
                     sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (::bind(listen_socket_.get(), reinterpret_cast<sockaddr *>(&addr),
                   sizeof(addr)) < 0)
        {
            perror("bind");
            return false;
        }

        if (::listen(listen_socket_.get(), 32) < 0)
        {
            perror("listen");
            return false;
        }

        // 第一个位置是监听 socket
        fds_.push_back({listen_socket_.get(), POLLIN, 0});

        std::cout << "Server listening on 127.0.0.1:" << port_ << '\n';

        return true;
    }

    void run()
    {
        while (true)
        {
            int ready =
                ::poll(fds_.data(), static_cast<nfds_t>(fds_.size()), -1);

            if (ready < 0)
            {
                if (errno == EINTR)
                    continue;

                perror("poll");
                break;
            }

            // 有新的连接请求
            if (fds_[0].revents & POLLIN)
                accept_client();

            // 从后往前遍历，方便删除元素
            for (std::size_t i = fds_.size(); i-- > 1;)
            {
                short events = fds_[i].revents;

                if (events & POLLIN)
                {
                    if (!read_client(i))
                        continue;
                }

                if (events & (POLLERR | POLLHUP | POLLNVAL))
                    remove_client(i);
            }
        }
    }

private:
    void accept_client()
    {
        int client_fd = ::accept(listen_socket_.get(), nullptr, nullptr);

        if (client_fd < 0)
        {
            perror("accept");
            return;
        }

        if (clients_.size() >= MAX_CLIENTS)
        {
            ::close(client_fd);
            return;
        }

        clients_.emplace_back(client_fd);

        fds_.push_back({client_fd, POLLIN, 0});

        std::cout << "Client connected: " << client_fd << '\n';
    }

    bool read_client(std::size_t index)
    {
        char buffer[BUFFER_SIZE];

        int fd = fds_[index].fd;

        ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);

        if (n > 0)
        {
            std::string_view msg(buffer, static_cast<std::size_t>(n));

            broadcast(index, msg);
            return true;
        }

        if (n < 0 && errno == EINTR)
            return true;

        remove_client(index);
        return false;
    }

    void broadcast(std::size_t sender, std::string_view msg)
    {
        for (std::size_t i = 1; i < fds_.size();)
        {
            if (i == sender)
            {
                ++i;
                continue;
            }

            if (!send_all(fds_[i].fd, msg))
            {
                remove_client(i);

                // 删除较小下标时，发送者位置也会变化
                if (i < sender)
                    --sender;
            }
            else
            {
                ++i;
            }
        }
    }

    void remove_client(std::size_t index)
    {
        std::cout << "Client disconnected: " << fds_[index].fd << '\n';

        // Socket 析构函数负责 close()
        clients_.erase(clients_.begin() + index - 1);
        fds_.erase(fds_.begin() + index);
    }

private:
    unsigned short port_;

    Socket listen_socket_;

    // clients_[i] 持有 fds_[i+1] 所对应的 fd
    std::vector<Socket> clients_;
    std::vector<pollfd> fds_;
};

// 4. 客户端
void run_client(const std::string &ip, unsigned short port)
{
    Socket sock(::socket(AF_INET, SOCK_STREAM, 0));

    if (sock.get() < 0)
    {
        perror("socket");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1)
    {
        std::cerr << "Invalid IPv4 address\n";
        return;
    }

    if (::connect(sock.get(), reinterpret_cast<sockaddr *>(&addr),
                  sizeof(addr)) < 0)
    {
        perror("connect");
        return;
    }

    std::cout << "Connected! Type messages:\n";

    pollfd fds[2] = {{STDIN_FILENO, POLLIN, 0}, {sock.get(), POLLIN, 0}};

    char buffer[BUFFER_SIZE];

    while (true)
    {
        int ready = ::poll(fds, 2, -1);

        if (ready < 0)
        {
            if (errno == EINTR)
                continue;
            perror("poll");
            break;
        }

        // 收到其他客户端的聊天消息
        if (fds[1].revents & POLLIN)
        {
            ssize_t n = ::recv(sock.get(), buffer, sizeof(buffer), 0);

            if (n > 0)
            {
                std::cout.write(buffer, n);
                std::cout.flush();
            }
            else if (n == 0)
            {
                break;
            }
            else if (errno != EINTR)
            {
                perror("recv");
                break;
            }
        }

        if (fds[1].revents & (POLLHUP | POLLERR | POLLNVAL))
            break;

        // 从终端读取用户输入
        if (fds[0].revents & POLLIN)
        {
            ssize_t n = ::read(STDIN_FILENO, buffer, sizeof(buffer));

            if (n > 0)
            {
                std::string_view msg(buffer, static_cast<std::size_t>(n));

                if (!send_all(sock.get(), msg))
                {
                    perror("send");
                    break;
                }
            }
            else if (n == 0)
            {
                // Ctrl+D：关闭发送方向
                ::shutdown(sock.get(), SHUT_WR);
                fds[0].fd = -1;
            }
            else if (errno != EINTR)
            {
                perror("read");
                break;
            }
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            fds[0].fd = -1;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage:\n"
                  << "  ./chat server PORT\n"
                  << "  ./chat client IP PORT\n";
        return 1;
    }

    char *end = nullptr;
    unsigned long value = std::strtoul(argv[argc - 1], &end, 10);

    if (!argv[argc - 1][0] || *end != '\0' || value > 65535)
    {
        std::cerr << "Invalid port\n";
        return 1;
    }

    auto port = static_cast<unsigned short>(value);
    std::string mode = argv[1];

    if (mode == "server" && argc == 3)
    {
        ChatServer server(port);

        if (!server.start())
            return 1;

        server.run();
    }
    else if (mode == "client" && argc == 4)
    {
        run_client(argv[2], port);
    }
    else
    {
        std::cerr << "Invalid arguments\n";
        return 1;
    }

    return 0;
}
