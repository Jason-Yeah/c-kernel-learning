// chat_mgr.cpp — 聊天管理实现
#include "chat_mgr.hpp"
#include "session.hpp"
#include "session_manager.hpp"'
#include <iostream>

void ChatMgr::handle_chat(const LogicMessage &msg)
{
    // 简化：直接把收到的内容 echo 回去
    auto sess = SessionManager::get_instance().get_session(msg.session_id);
    if (sess)
    {
        sess->send(msg.body);
        std::cout << "[chat] echoed " << msg.body.size() << " bytes to session "
                  << msg.session_id << "\n";
    }
}

void ChatMgr::on_user_online(uint64_t user_id)
{
    std::cout << "[chat] user " << user_id << " is now online\n";
}

void ChatMgr::on_user_offline(uint64_t user_id)
{
    std::cout << "[chat] user " << user_id << " is now offline\n";
}
