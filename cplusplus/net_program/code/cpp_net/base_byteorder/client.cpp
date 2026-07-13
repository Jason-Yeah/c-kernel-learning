// ============================================================
// file: base_byteorder/client.cpp
// TLV Echo Client — 演示网络字节序的处理
// 协议：[ Type:2B (htons) ][ BodyLen:4B (htonl) ][ Body:N B ]
// ============================================================
#include <array>
#include <boost/asio.hpp>
#include <iostream>
#include <vector>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

constexpr uint16_t PORT = 9190;
constexpr size_t HEADER_SIZE = 6;

// ============================================================
// 发送一条 TLV 消息
// 演示：发送前 htons/htonl
// ============================================================
void send_message(tcp::socket &sock, uint16_t type,
                  const std::vector<char> &body)
{
    // ===== 字节序转换：主机字节序 → 网络字节序 =====
    uint16_t net_type = htons(type);       // ← 转网络字节序
    uint32_t net_len = htonl(body.size()); // ← 转网络字节序

    // 组装 buffer 一次发完
    std::array<asio::const_buffer, 3> bufs = {
        asio::buffer(&net_type, 2), // type（网络字节序）
        asio::buffer(&net_len, 4),  // body_len（网络字节序）
        asio::buffer(body)          // body 本身不转
    };
    asio::write(sock, bufs);
}

// ============================================================
// 接收一条 TLV 消息（同步阻塞，用于教学演示）
// 演示：收到后 ntohs/ntohl
// ============================================================
struct Message
{
    uint16_t type;
    std::vector<char> body;
};

Message recv_message(tcp::socket &sock)
{
    Message msg;

    // 1. 读 6 字节 header, 固定大小协议头可用std::array
    std::array<char, HEADER_SIZE> header;
    asio::read(sock, asio::buffer(header));

    // ===== 字节序转换：网络字节序 → 主机字节序 =====
    uint16_t net_type;
    memcpy(&net_type, header.data(), 2);
    msg.type = ntohs(net_type); // ← 转回主机字节序

    uint32_t net_len;
    memcpy(&net_len, header.data() + 2, 4);
    uint32_t body_len = ntohl(net_len); // ← 转回主机字节序

    // 2. 读 body
    if (body_len > 0)
    {
        // resize把std容器的大小重新设置了，实际元素量
        msg.body.resize(body_len);
        asio::read(sock, asio::buffer(msg.body));
    }

    return msg;
}

// ============================================================
// main — 连接服务器，发送一条消息，接收 echo 回复
// ============================================================
int main()
{
    try
    {
        asio::io_context io;
        tcp::socket sock(io);

        // 连接服务器
        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve("127.0.0.1", std::to_string(PORT));
        asio::connect(sock, endpoints);
        std::cout << "[client] connected to 127.0.0.1:" << PORT << "\n";

        // ===== 发送一条消息（type=1, body="HelloByteOrder"）=====
        std::string payload = "HelloByteOrder";
        std::vector<char> body(payload.begin(), payload.end());

        std::cout << "[client] sending: type=" << 1 << ", body=\"" << payload
                  << "\"" << ", body_len=" << body.size() << "\n";
        send_message(sock, 1, body);

        // ===== 接收 echo 回复 =====
        Message reply = recv_message(sock);
        std::string reply_str(reply.body.begin(), reply.body.end());
        std::cout << "[client] received: type=" << reply.type << ", body=\""
                  << reply_str << "\"" << ", body_len=" << reply.body.size()
                  << "\n";

        // ===== 验证 echo 正确 =====
        if (reply.type == 1 && reply.body == body)
        {
            std::cout << "[client] echo OK — 字节序处理正确\n";
        }
        else
        {
            std::cout << "[client] echo MISMATCH!\n";
        }

        sock.close();
        return 0;
    }
    catch (std::exception &e)
    {
        std::cerr << "[client] error: " << e.what() << "\n";
        return 1;
    }
}
