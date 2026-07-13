// login_mgr.cpp — 登录管理实现
#include "login_mgr.hpp"
#include "session_manager.hpp"
#include "chat_mgr.hpp"
#include <iostream>

void LoginMgr::handle_login(const LogicMessage& msg)
{
    // 简化：从 body 解析用户名（实际项目中会是 protobuf/JSON）
    std::string username(msg.body.begin(), msg.body.end());
    uint64_t user_id = std::hash<std::string>{}(username);

    // 记录在线状态
    _online_users[user_id] = msg.session_id;
    _session_to_user[msg.session_id] = user_id;

    std::cout << "[login] user " << username
              << " (id=" << user_id << ") logged in, session="
              << msg.session_id << "\n";

    // 通知聊天模块：用户上线
    ChatMgr::get_instance().on_user_online(user_id);
}

void LoginMgr::handle_logout(uint64_t session_id)
{
    auto it = _session_to_user.find(session_id);
    if (it == _session_to_user.end()) return;

    uint64_t user_id = it->second;
    _online_users.erase(user_id);
    _session_to_user.erase(it);

    std::cout << "[login] user " << user_id << " logged out\n";
}

bool LoginMgr::is_online(uint64_t user_id) const
{
    return _online_users.find(user_id) != _online_users.end();
}

uint64_t LoginMgr::get_user_by_session(uint64_t session_id) const
{
    auto it = _session_to_user.find(session_id);
    return it != _session_to_user.end() ? it->second : 0;
}
