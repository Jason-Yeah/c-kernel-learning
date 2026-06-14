
#include <chrono>
#include <ctime>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <string>
#include "logger.h"

logger& logger::instance()
{
    // C++11: The first init of thread security.
    static logger _inst;
    return _inst;
}

logger::logger()
{
    _log_file.open("log.txt", std::ios::app | std::ios::out);
    if (!_log_file.is_open())   throw std::runtime_error("Failed to open log file.");
}

logger::~logger()
{
    if (_log_file.is_open()) _log_file.close();
}

void logger::log(const std::string& msg)
{
    // 加锁防止多线程调用出岔子
    std::lock_guard<std::mutex> lock(_mtx);
    if (_log_file.is_open())
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf = log_time_util::localtime_safe(t);

        char buf[LOG_TIME_SIZE];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);

        _log_file << "\"" << std::string(buf) << "\"" << " : " << msg << std::endl;
    }
}

