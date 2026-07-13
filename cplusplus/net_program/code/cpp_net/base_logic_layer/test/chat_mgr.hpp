// chat_mgr.hpp — 聊天管理（单例）
#pragma once

#include <cstdint>
#include "singleton.hpp"
#include "message.hpp"

class ChatMgr : public Singleton<ChatMgr> {
    friend class Singleton<ChatMgr>;
public:
    // 处理聊天消息
    void handle_chat(const LogicMessage& msg);

    // 用户上线通知（由 LoginMgr 调用）
    void on_user_online(uint64_t user_id);

    // 用户下线通知（由 LoginMgr 调用）
    void on_user_offline(uint64_t user_id);

private:
    ChatMgr() = default;
};
