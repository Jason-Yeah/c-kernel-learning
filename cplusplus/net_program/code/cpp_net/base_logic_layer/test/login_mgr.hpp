// login_mgr.hpp — 登录管理（单例）
#pragma once

#include <unordered_map>
#include <cstdint>
#include "singleton.hpp"
#include "message.hpp"

class LoginMgr : public Singleton<LoginMgr> {
    friend class Singleton<LoginMgr>;
public:
    // 处理登录请求
    void handle_login(const LogicMessage& msg);

    // 处理登出
    void handle_logout(uint64_t session_id);

    // 检查用户是否在线
    bool is_online(uint64_t user_id) const;

    // 通过 Session ID 查找用户 ID
    uint64_t get_user_by_session(uint64_t session_id) const;

private:
    LoginMgr() = default;

    // 在线用户表：user_id → session_id
    std::unordered_map<uint64_t, uint64_t> _online_users;
    // 反向映射：session_id → user_id
    std::unordered_map<uint64_t, uint64_t> _session_to_user;
};
