// network_server.hpp — 网络层
// 职责：io_context、acceptor、信号处理、顶层组装
#pragma once

#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <cstdint>
#include "config.h"

using boost::asio::ip::tcp;

class NetworkServer {
public:
    explicit NetworkServer(uint16_t port = SERVER_PORT);
    ~NetworkServer();

    // 启动（阻塞，直到优雅退出）
    void run();

    // 停止
    void stop();

private:
    // 注册信号处理（优雅退出）
    void setup_signals();

    // accept 新连接
    void do_accept();

    // 启动逻辑线程
    void start_logic_thread();

    boost::asio::io_context       _io;
    tcp::acceptor                 _acceptor;
    boost::asio::signal_set       _signals;
    std::thread                   _logic_thread;
    std::atomic<uint64_t>         _next_id{1};
};
