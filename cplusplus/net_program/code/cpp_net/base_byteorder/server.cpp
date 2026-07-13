// ============================================================
// file: base_byteorder/server.cpp
// TLV Echo Server — 演示网络字节序的完整处理
// 协议：[ Type:2B (htons) ][ BodyLen:4B (htonl) ][ Body:N B ]
// ============================================================
#include <array>
#include <boost/asio.hpp>
#include <cstdint> // uint32_t, uint16_t
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

constexpr uint16_t PORT = 9190;
constexpr size_t HEADER_SIZE = 6;              // type(2) + body_len(4)
constexpr uint32_t MAX_BODY = 4 * 1024 * 1024; // 4MB 安全上限

// ============================================================
// Session：处理一个客户端的完整生命周期
// 演示：接收时 ntohs/ntohl，发送时 htons/htonl
// ============================================================
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket) : _sock(std::move(socket)) {}

    void start()
    {
        std::cout << "[session] new connection from " << _sock.remote_endpoint()
                  << std::endl;
        read_header();
    }

private:
    // ----------------------------------------------------------
    // 第一步：读 6 字节 header
    // ----------------------------------------------------------
    void read_header()
    {
        auto self = shared_from_this();
        asio::async_read(_sock, asio::buffer(_header_buf),
                         [self](boost::system::error_code ec, size_t)
                         {
                             if (ec)
                             {
                                 self->handle_error(ec);
                                 return;
                             }

                             // ===== 字节序转换：从网络字节序到主机字节序 =====
                             // _header_buf 布局：[ type(2) ][ body_len(4) ]
                             //   → 从网络收到的是大端
                             //   → 转成当前主机的小端才能用

                             // 前 2 字节：type (uint16_t, 大端)
                             uint16_t net_type;
                             memcpy(&net_type, self->_header_buf.data(), 2);
                             uint16_t host_type =
                                 ntohs(net_type); // ← 网络转主机

                             // 后 4 字节：body_len (uint32_t, 大端)
                             uint32_t net_len;
                             memcpy(&net_len, self->_header_buf.data() + 2, 4);
                             uint32_t host_len = ntohl(net_len); // ← 网络转主机

                             // 安全检查：防止恶意大包
                             if (host_len > MAX_BODY)
                             {
                                 std::cerr
                                     << "[session] body too large: " << host_len
                                     << ", closing\n";
                                 return;
                             }

                             self->read_body(host_type, host_len);
                         });
    }

    // ----------------------------------------------------------
    // 第二步：读 body（长度已知）
    // ----------------------------------------------------------
    void read_body(uint16_t type, uint32_t len)
    {
        auto self = shared_from_this();
        auto body = std::make_shared<std::vector<char>>(len);

        if (len == 0)
        {
            // body 为空，直接 echo 回去
            self->echo_back(type, body);
            return;
        }

        asio::async_read(
            _sock, asio::buffer(*body),
            [self, type, body](boost::system::error_code ec, size_t)
            {
                if (ec)
                {
                    self->handle_error(ec);
                    return;
                }
                self->echo_back(type, body);
            });
    }

    // ----------------------------------------------------------
    // Echo：收到什么发回什么（字节序也要正确处理）
    // ----------------------------------------------------------
    void echo_back(uint16_t type, std::shared_ptr<std::vector<char>> body)
    {
        // ===== 字节序转换：从主机字节序到网络字节序 =====
        // type 和 body_len 发出去之前要转成大端（网络字节序）
        uint16_t net_type = htons(type);        // ← 主机转网络
        uint32_t net_len = htonl(body->size()); // ← 主机转网络

        // 组装三个 buffer 一次发完（scatter-gather）
        std::array<asio::const_buffer, 3> bufs = {
            asio::buffer(&net_type, 2), // type（已转网络字节序）
            asio::buffer(&net_len, 4),  // body_len（已转网络字节序）
            asio::buffer(*body) // body 本身不转（二进制/字符串）
        };

        auto self = shared_from_this();
        asio::async_write(_sock, bufs,
                          [self, body](boost::system::error_code ec, size_t)
                          {
                              if (ec)
                              {
                                  self->handle_error(ec);
                                  return;
                              }
                              // echo 完成，继续读下一个消息
                              self->read_header();
                          });
    }

    void handle_error(boost::system::error_code ec)
    {
        if (ec == asio::error::eof)
        {
            std::cout << "[session] client closed\n";
        }
        else
        {
            std::cerr << "[session] error: " << ec.message() << "\n";
        }
    }

    tcp::socket _sock;
    std::array<char, HEADER_SIZE> _header_buf;
};

// ============================================================
// Server：接受连接，创建 Session
// ============================================================
class Server
{
public:
    Server(asio::io_context &io, uint16_t port)
        : _acceptor(io, tcp::endpoint(tcp::v4(), port))
    {
        do_accept();
        std::cout << "[server] listening on 0.0.0.0:" << port << "\n";
    }

private:
    void do_accept()
    {
        auto sock = std::make_shared<tcp::socket>(_acceptor.get_executor());
        _acceptor.async_accept(
            *sock,
            [this, sock](boost::system::error_code ec)
            {
                if (!ec)
                {
                    std::make_shared<Session>(std::move(*sock))->start();
                }
                do_accept();
            });
    }

    tcp::acceptor _acceptor;
};

// ============================================================
// main：多线程 io_context
// ============================================================
int main()
{
    try
    {
        asio::io_context io;
        Server server(io, PORT);

        // ===== 多线程跑 io_context =====
        // hardware_concurrency() 返回逻辑 CPU 核数（建议值，可能返回 0）
        // 网络 I/O 密集程序不需要太多线程，2~4 个通常就够
        // 规则：至少 1，最多不超过 4 个额外线程
        unsigned int suggested = std::thread::hardware_concurrency();
        unsigned int extra_threads = 0;
        if (suggested > 0) {
            extra_threads = std::min(suggested - 1, 3u); // 最多 3 个额外线程
        }
        // 主线程自己也会跑 io.run()，额外线程负责分担负载

        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < extra_threads; ++i)
        {
            threads.emplace_back([&io] { io.run(); });
        }
        io.run(); // 主线程也跑
        for (auto &t : threads)
            t.join();
    }
    catch (std::exception &e)
    {
        std::cerr << "[server] fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
