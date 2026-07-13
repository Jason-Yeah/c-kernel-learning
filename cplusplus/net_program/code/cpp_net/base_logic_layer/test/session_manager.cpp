// session_manager.cpp — SessionManager 实现
#include "session_manager.hpp"
#include "session.hpp"
#include <iostream>

void SessionManager::add_session(uint64_t id, std::shared_ptr<Session> sess)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions[id] = std::move(sess);
}

void SessionManager::remove_session(uint64_t id)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(id);
}

std::shared_ptr<Session> SessionManager::get_session(uint64_t id)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(id);
    return it != _sessions.end() ? it->second : nullptr;
}

void SessionManager::close_all()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::cout << "[session_mgr] closing " << _sessions.size()
              << " sessions...\n";

    for (auto &[id, sess] : _sessions)
        sess->close();

    _sessions.clear();
    std::cout << "[session_mgr] all sessions closed\n";
}

size_t SessionManager::count() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _sessions.size();
}
