// logic_system.cpp — LogicSystem 实现
#include "logic_system.hpp"
#include "session.hpp"
#include "session_manager.hpp"
#include <iostream>

void LogicSystem::post_message(uint64_t session_id, uint16_t msg_type,
                               const std::vector<char> &body)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.push(LogicMessage(session_id, msg_type, body));
    _cond.notify_one();
}

void LogicSystem::run()
{
    std::cout << "[logic] worker started\n";
    while (!_stop)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this] { return !_queue.empty() || _stop; });

        if (_stop)
            break;

        auto msg = std::move(_queue.front());
        _queue.pop();
        lock.unlock();

        dispatch(msg);
    }
    std::cout << "[logic] worker stopped\n";
}

void LogicSystem::stop()
{
    _stop = true;
    _cond.notify_all();
}

void LogicSystem::register_handler(uint16_t msg_type, MsgHandler handler)
{
    _handlers[msg_type] = std::move(handler);
}

void LogicSystem::dispatch(const LogicMessage &msg)
{
    auto it = _handlers.find(msg.msg_type);
    if (it != _handlers.end())
    {
        it->second(msg);
    }
    else
    {
        // 没有注册处理器 → echo 回去（默认行为）
        auto sess = SessionManager::get_instance().get_session(msg.session_id);
        if (sess)
        {
            sess->send(msg.body);
        }
    }
}
