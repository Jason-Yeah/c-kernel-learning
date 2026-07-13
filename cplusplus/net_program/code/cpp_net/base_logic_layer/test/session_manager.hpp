// session_manager.hpp — SessionManager（单例）
// 管理所有活跃的 Session 连接
#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <cstdint>
#include "singleton.hpp"

class Session;

class SessionManager : public Singleton<SessionManager> {
    friend class Singleton<SessionManager>;
public:
    // 注册 Session
    void add_session(uint64_t id, std::shared_ptr<Session> sess);

    // 移除 Session
    void remove_session(uint64_t id);

    // 获取某个 Session（用于回复消息）
    std::shared_ptr<Session> get_session(uint64_t id);

    // 关闭所有 Session（优雅退出时调用）
    void close_all();

    // 当前活跃连接数
    size_t count() const;

private:
    SessionManager() = default;

    mutable std::mutex _mutex;
    std::unordered_map<uint64_t, std::shared_ptr<Session>> _sessions;
    uint64_t _next_id = 1;
};
