// echo_server.cpp — ASIO + protobuf + TLV 粘包
// 协议：[ total:4B ][ msg_type:2B ][ protobuf body:N B ]
//       htonl         htons          自动处理
#include "chat.pb.h"
#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <iostream>
#include <memory>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

constexpr uint16_t PORT = 9191;
constexpr size_t HEADER_SIZE = 6; // total(4) + type(2)
constexpr uint32_t MAX_BODY = 4 * 1024 * 1024;

// ===== 消息类型枚举 =====
enum MsgType : uint16_t
{
    LOGIN_REQUEST = 1,
    LOGIN_RESPONSE = 2,
    CHAT_MESSAGE = 3,
    HEARTBEAT = 4,
};

// ===== 接收一条 protobuf 消息 =====
// 返回 true 成功，false 失败（连接断开或非法数据）
bool recv_protobuf(tcp::socket &sock, std::string &body_out,
                   uint16_t &msg_type_out)
{
    // 1. 读 6B header
    std::array<char, HEADER_SIZE> header;
    boost::system::error_code ec;
    size_t n = asio::read(sock, asio::buffer(header), ec);
    if (ec)
        return false;

    // 2. 解析 header（hton 转为主机字节序）
    uint32_t net_total;
    memcpy(&net_total, header.data(), 4);
    uint32_t total = ntohl(net_total);

    uint16_t net_type;
    memcpy(&net_type, header.data() + 4, 2);
    msg_type_out = ntohs(net_type);

    if (total < HEADER_SIZE || total > MAX_BODY)
    {
        return false; // 非法长度
    }

    // 3. 读 body
    uint32_t body_len = total - HEADER_SIZE;
    body_out.resize(body_len);
    if (body_len > 0)
    {
        asio::read(sock, asio::buffer(body_out), ec);
        if (ec)
            return false;
    }
    return true;
}

// ===== 发送一条 protobuf 消息 =====
void send_protobuf(tcp::socket &sock, uint16_t msg_type,
                   const google::protobuf::Message &msg)
{
    // 1. 序列化
    std::string body;
    if (!msg.SerializeToString(&body)) {
        std::cerr << "[send] serialization failed\n";
        return;
    }

    // 2. 拼 TLV header
    uint32_t total = HEADER_SIZE + body.size();
    uint32_t net_total = htonl(total);
    uint16_t net_type = htons(msg_type);

    std::array<asio::const_buffer, 3> bufs = {asio::buffer(&net_total, 4),
                                              asio::buffer(&net_type, 2),
                                              asio::buffer(body)};
    asio::write(sock, bufs);
}

// ===== Session：处理一个客户端 =====
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket) : _sock(std::move(socket)) {}
    void start() { read_loop(); }

private:
    void read_loop()
    {
        auto self = shared_from_this();
        // 粘包自动处理：内部读 6B header → 读 body
        boost::asio::async_read(_sock, asio::buffer(_header_buf),
                                [self](boost::system::error_code ec, size_t)
                                {
                                    if (ec)
                                    { /* 断开 */
                                        return;
                                    }
                                    self->on_header();
                                });
    }

    void on_header()
    {
        // 解析 header
        uint32_t net_total;
        memcpy(&net_total, _header_buf.data(), 4);
        uint32_t total = ntohl(net_total);

        uint16_t net_type;
        memcpy(&net_type, _header_buf.data() + 4, 2);
        _msg_type = ntohs(net_type);

        if (total < HEADER_SIZE || total > MAX_BODY)
        {
            return;
        }

        uint32_t body_len = total - HEADER_SIZE;

        if (body_len == 0)
        {
            // 空 body，直接处理
            on_body(std::make_shared<std::string>());
            return;
        }

        // 读 body
        auto body = std::make_shared<std::string>();
        body->resize(body_len);
        auto self = shared_from_this();
        boost::asio::async_read(
            _sock, asio::buffer(*body),
            [self, body](boost::system::error_code ec, size_t)
            {
                if (ec)
                    return;
                self->on_body(body);
            });
    }

    void on_body(std::shared_ptr<std::string> raw)
    {
        // 根据消息类型反序列化并处理
        switch (_msg_type)
        {
        case CHAT_MESSAGE:
        {
            chat::ChatMessage msg;
            if (msg.ParseFromString(*raw))
            {
                std::cout << "[CHAT] " << msg.sender() << " → "
                          << msg.receiver() << ": " << msg.content() << "\n";

                // echo 回去
                send_protobuf(_sock, CHAT_MESSAGE, msg);
            }
            break;
        }
        case HEARTBEAT:
        {
            chat::Heartbeat hb;
            if (hb.ParseFromString(*raw))
            {
                // 回复心跳
                send_protobuf(_sock, HEARTBEAT, hb);
            }
            break;
        }
        default:
            std::cerr << "[session] unknown msg type: " << _msg_type << "\n";
            break;
        }

        // 继续读下一个消息
        read_loop();
    }

    tcp::socket _sock;
    std::array<char, HEADER_SIZE> _header_buf;
    uint16_t _msg_type = 0;
};

// ===== Server =====
class Server
{
public:
    Server(asio::io_context &io, uint16_t port)
        : _acceptor(io, tcp::endpoint(tcp::v4(), port))
    {
        do_accept();
        std::cout << "[server] protobuf echo on 0.0.0.0:" << port << "\n";
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
                    std::make_shared<Session>(std::move(*sock))->start();
                do_accept();
            });
    }
    tcp::acceptor _acceptor;
};

int main()
{
    try
    {
        asio::io_context io;
        Server srv(io, PORT);
        io.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
