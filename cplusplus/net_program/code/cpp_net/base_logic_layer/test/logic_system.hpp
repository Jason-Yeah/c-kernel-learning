// logic_system.hpp — LogicSystem（逻辑层单例）
// 消息队列 + 消息分发 + 业务处理
#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <vector>
#include <functional>
#include "singleton.hpp"
#include "message.hpp"

// 消息处理回调类型
using MsgHandler = std::function<void(const LogicMessage&)>;

class LogicSystem : public Singleton<LogicSystem> {
    friend class Singleton<LogicSystem>;
public:
    // 投递消息（由 Session 调用，跨线程安全）
    void post_message(uint64_t session_id, uint16_t msg_type,
                      const std::vector<char>& body);

    // 处理消息循环（由逻辑线程调用，阻塞直到 stop）
    void run();

    // 停止消息循环
    void stop();

    // 注册消息处理器
    void register_handler(uint16_t msg_type, MsgHandler handler);

private:
    LogicSystem() = default;

    // 分发消息到对应的处理器
    void dispatch(const LogicMessage& msg);

    std::queue<LogicMessage>    _queue;
    std::mutex                  _mutex;
    std::condition_variable     _cond;
    std::atomic<bool>           _stop{false};

    // 消息类型 → 处理器映射表
    std::unordered_map<uint16_t, MsgHandler> _handlers;
};
