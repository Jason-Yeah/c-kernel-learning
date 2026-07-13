// session.hpp — Session 会话层
// 职责：单个连接的收发、TLV 粘包解包、投递完整消息到逻辑层
#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <array>
#include <vector>
#include <cstdint>
#include "config.h"

// 前置声明，避免循环包含
class LogicSystem;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::ip::tcp::socket socket, uint64_t id);
    ~Session();

    uint64_t id() const { return _id; }

    // 启动会话：注册到 SessionManager + 开始读消息
    void start();

    // 优雅关闭：先 shutdown 再 close
    void close();

    // 发送数据（由逻辑层调用）
    void send(const std::vector<char>& data);

private:
    // 第一步：读 HEAD_LEN 字节 header → 得到 body 长度
    void read_header();

    // 第二步：读 body_len 字节 → 得到完整消息
    void read_body(uint32_t body_len);

    // 收到完整消息 → 投递到 LogicSystem
    void on_message(const std::vector<char>& body);

    // 错误处理
    void handle_error(const boost::system::error_code& ec);

    boost::asio::ip::tcp::socket _sock;
    uint64_t                     _id;
    std::array<char, HEAD_LEN>   _header_buf;    // type(1) + length(4)
};
