
#ifndef TASK_LOGGER_H
#define TASK_LOGGER_H

#include <string>
#include <mutex>
#include <fstream>

const size_t LOG_TIME_SIZE = 50;

namespace log_time_util 
{
    static std::tm localtime_safe(std::time_t t)
    {
        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif
        return tm_buf;
    }
}

class logger
{
    public:

        static logger& instance();

        void log(const std::string& msg);

        ~logger();    

    private:

        logger();

        logger(const logger&) = delete;

        logger& operator= (const logger&) = delete;

    private:
        std::ofstream _log_file;
        std::mutex _mtx;        
};

#endif  // TASK_LOGGER_H
