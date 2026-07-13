// echo_client.cpp — ASIO + protobuf 客户端
#include "chat.pb.h"
#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <iostream>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

constexpr uint16_t PORT = 9191;
constexpr size_t HEADER_SIZE = 6;

enum MsgType : uint16_t
{
    LOGIN_REQUEST = 1,
    LOGIN_RESPONSE = 2,
    CHAT_MESSAGE = 3,
    HEARTBEAT = 4,
};

bool send_protobuf(tcp::socket &sock, uint16_t msg_type,
                   const google::protobuf::Message &msg)
{
    std::string body;
    if (!msg.SerializeToString(&body)) {
        std::cerr << "[send] serialization failed\n";
        return false;
    }

    uint32_t total = HEADER_SIZE + body.size();
    uint32_t net_total = htonl(total);
    uint16_t net_type = htons(msg_type);

    std::array<asio::const_buffer, 3> bufs = {asio::buffer(&net_total, 4),
                                              asio::buffer(&net_type, 2),
                                              asio::buffer(body)};
    asio::write(sock, bufs);
    return true;
}

bool recv_protobuf(tcp::socket &sock, std::string &body_out,
                   uint16_t &msg_type_out)
{
    std::array<char, HEADER_SIZE> header;
    boost::system::error_code ec;
    asio::read(sock, asio::buffer(header), ec);
    if (ec)
        return false;

    uint32_t total = ntohl(*(uint32_t *)header.data());
    msg_type_out = ntohs(*(uint16_t *)(header.data() + 4));

    uint32_t body_len = total - HEADER_SIZE;
    body_out.resize(body_len);
    if (body_len > 0)
        asio::read(sock, asio::buffer(body_out), ec);
    return !ec;
}

int main()
{
    try
    {
        asio::io_context io;
        tcp::socket sock(io);

        tcp::resolver resolver(io);
        auto eps = resolver.resolve("127.0.0.1", std::to_string(PORT));
        asio::connect(sock, eps);
        std::cout << "[client] connected\n";

        // 1. 发送 ChatMessage
        chat::ChatMessage msg;
        msg.set_sender(1001);
        msg.set_receiver(2002);
        msg.set_content("Hello from protobuf client!");

        std::cout << "[client] sending: " << msg.sender() << " → "
                  << msg.receiver() << " \"" << msg.content() << "\"\n";
        send_protobuf(sock, CHAT_MESSAGE, msg);

        // 2. 接收 echo 回复
        uint16_t reply_type;
        std::string raw;
        if (recv_protobuf(sock, raw, reply_type))
        {
            chat::ChatMessage reply;
            if (reply.ParseFromString(raw))
            {
                std::cout << "[client] echo back: " << reply.sender() << " → "
                          << reply.receiver() << " \"" << reply.content()
                          << "\"\n";
                std::cout << "[client] protobuf echo OK\n";
            }
        }

        sock.close();
    }
    catch (std::exception &e)
    {
        std::cerr << "[client] error: " << e.what() << "\n";
        return 1;
    }
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
