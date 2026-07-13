// network_server.cpp — 网络层实现
#include "network_server.hpp"
#include "session.hpp"
#include "session_manager.hpp"
#include "logic_system.hpp"
#include "login_mgr.hpp"
#include "chat_mgr.hpp"
#include <iostream>

NetworkServer::NetworkServer(uint16_t port)
    : _acceptor(_io, tcp::endpoint(tcp::v4(), port))
    , _signals(_io)
{
    // 注册消息处理器到 LogicSystem
    auto& logic = LogicSystem::get_instance();
    logic.register_handler(MSG_LOGIN, [](const LogicMessage& msg) {
        LoginMgr::get_instance().handle_login(msg);
    });
    logic.register_handler(MSG_CHAT, [](const LogicMessage& msg) {
        ChatMgr::get_instance().handle_chat(msg);
    });
    // MSG_HEARTBEAT、MSG_LOGOUT 等可以在这里继续注册

    setup_signals();
    do_accept();
    start_logic_thread();

    std::cout << "[server] listening on 0.0.0.0:" << port << "\n";
}

NetworkServer::~NetworkServer()
{
    stop();
}

void NetworkServer::run()
{
    _io.run();
    // io.run() 返回后 → 等待逻辑线程结束
    if (_logic_thread.joinable()) {
        _logic_thread.join();
    }
    std::cout << "[server] exited cleanly\n";
}

void NetworkServer::stop()
{
    _acceptor.close();
    SessionManager::get_instance().close_all();
    LogicSystem::get_instance().stop();
    _io.stop();
}

// ============================================================
// 内部实现
// ============================================================

void NetworkServer::setup_signals()
{
    _signals.add(SIGINT);
    _signals.add(SIGTERM);
    _signals.async_wait([this](boost::system::error_code ec, int sig) {
        if (ec) return;
        std::cout << "\n[server] signal " << sig
                  << " received, shutting down...\n";
        stop();
    });
}

void NetworkServer::do_accept()
{
    auto sock = std::make_shared<tcp::socket>(_io);
    _acceptor.async_accept(*sock,
        [this, sock](boost::system::error_code ec) {
            if (!ec) {
                auto id = _next_id++;
                std::make_shared<Session>(std::move(*sock), id)->start();
            }
            if (!_acceptor.is_open()) return;
            do_accept();
        });
}

void NetworkServer::start_logic_thread()
{
    _logic_thread = std::thread([] {
        LogicSystem::get_instance().run();
    });
}
