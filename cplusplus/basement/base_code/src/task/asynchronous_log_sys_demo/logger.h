
#ifndef LOGGER_DEMO_H
#define LOGGER_DEMO_H

#include <chrono>
#include <cstddef>
#include <ctime>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <mutex>
#include <sys/types.h>
#include <thread>
#include <condition_variable>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

#define TIME_BUF_SIZE 50

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

/**
* 将单个参数转为字符串
* 采用oss是未了更通用与类型安全。只要T类型重载了<<运算符，就可被处理
*/
template<typename T>
std::string to_str_helper(T&& arg)
{
    std::ostringstream oss;
    oss << std::forward<T>(arg);
    return oss.str();
}

class log_queue
{
    public:
        bool push(const std::string& msg)
        {
            {
                std::lock_guard<std::mutex> lock(_mtx);

                if (_is_shutdown) return false;               
                _queue.push(std::move(msg));
            }

            _cond_var.notify_one();
            return true;
        }

        bool pop(std::string& msg)
        {
            std::unique_lock<std::mutex> lock(_mtx);

            _cond_var.wait(lock, [this](){
                return !_queue.empty() || _is_shutdown;
            });

            // 唤醒后再次确认如果队列为空且程序要退出，就返回false
            if (_queue.empty() && _is_shutdown) return false; 

            msg = std::move(_queue.front());
            _queue.pop();
            return true;
        }

        /**
        * 通知所有线程程序即将退出
        */
        void shutdown()
        {
            {
                // 保证原子操作
                std::lock_guard<std::mutex> lock(_mtx);
                _is_shutdown = true;
            }
            _cond_var.notify_all();
        }

    private:
        std::queue<std::string> _queue;
        std::mutex _mtx;
        std::condition_variable _cond_var;
        bool _is_shutdown = false;
};

enum class log_level
{
    INFO, DEBUG, WARN, ERROR
};

// 所有输出组件都要继承该组件
struct log_sink
{
    virtual ~log_sink() = default;
    virtual void write(const std::string& msg) = 0;
};

class console_sink: public log_sink
{
    public:
        void write(const std::string& msg) override // 表示是重写不是独立的新函数
        {
            // 控制台直接使用std::cout输出到控制台即可。
            std::cout << msg << std::endl;
        }
};

class rotating_file_sink: public log_sink
{
    public:
        // "log_2026-06-11_0.txt"
        rotating_file_sink(std::string base_name, size_t max_byte, 
                           bool rotate_by_date = true,
                           std::string ext = ".txt"):
                           _base_name(base_name), _max_byte(max_byte), _rotate_by_date(rotate_by_date),
                           _ext(std::move(ext))
        {
            _curr_date = get_curr_date();
            open_file();
        }

        ~rotating_file_sink() override
        {
            if (_ofs.is_open()) _ofs.close();
        }

        void write(const std::string& msg) override
        {
            std::string line = msg + '\n';

            rotate_if_need(line.size());

            _ofs << line;
            _ofs.flush();

            // 统计当前日志大小
            _curr_size += line.size();
        }
    private:
        std::string _base_name;
        std::string _ext;
        std::ofstream _ofs;
        
        size_t _max_byte;
        size_t _curr_size = 0;

        bool _rotate_by_date;
        std::string _curr_date;
        size_t _file_idx = 0;
    
    private:

        std::string get_curr_date()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf = log_time_util::localtime_safe(t);

            char buf[TIME_BUF_SIZE];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm_buf);
            return std::string(buf);
        }

        std::string make_file_name() const
        {
            std::ostringstream oss;
            // _curr_date = get_curr_date()
            oss << _base_name << "_" << _curr_date << "_" << _file_idx << _ext;
            return oss.str();
        }

        // 仅构造时和写入时调用
        void open_file()
        {
            if (_ofs.is_open()) _ofs.close();

            std::string filename = make_file_name();
            _ofs.open(filename, std::ios::app | std::ios::out);

            if (!_ofs.is_open()) throw std::runtime_error("Failed to open log files: " + filename);
            // seek put移动输入流的写入指针，控制接下来写入文件的位置。
            // seekp(off, dir)从文件dir开始偏移多少位置写，这里相当于表示从当前文件末尾开始
            _ofs.seekp(0, std::ios::end);
            auto pos = _ofs.tellp();

            // 检查操作是否失败，例如文件是否未成功打开或流处于错误状态
            if (pos == std::streampos(-1))
                _curr_size = 0;
            else
                _curr_size = static_cast<size_t>(pos);
        }

        void rotate_if_need(size_t next_writ_size)
        {
            std::string today = get_curr_date();
            if (_rotate_by_date && today != _curr_date)
            {
                _curr_date = today;
                _file_idx = 0;
                open_file();
                return ;
            }

            if (_max_byte > 0 && _curr_size + next_writ_size > _max_byte)
            {
                ++ _file_idx;
                open_file();
            }
        }
};

class logger
{
    public:
        logger()
        {
            _worker_thread = std::thread([this](){
                process_queue();
            });
        } 

        ~logger()
        {
            _lqueue.shutdown();
            if (_worker_thread.joinable())  _worker_thread.join();
        }

        void add_sink(std::unique_ptr<log_sink> sink) // 只能移动不可拷贝
        {
            std::lock_guard<std::mutex> lock(_sink_mtx);
            _sinks.push_back(std::move(sink));
        }

        // 转发引用
        template<typename ... Args>
        void log(log_level level, const std::string& format, Args&&... args)
        {
            std::string str = level_to_str(level) + format_msg(format, std::forward<Args>(args)...);
            _lqueue.push(std::move(str));
        }
    private:
        log_queue _lqueue;
        std::thread _worker_thread;
        std::mutex _sink_mtx;
        std::vector<std::unique_ptr<log_sink>> _sinks;

    private:
        void process_queue()
        {
            std::string msg;

            while(_lqueue.pop(msg))
            {
                std::lock_guard<std::mutex> lock(_sink_mtx);

                for (auto& sink: _sinks)
                    if (sink)   sink->write(msg);
            }
        } 

        static std::string level_to_str(log_level level)
        {
            std::string level_str;
            switch (level) 
            {
                case log_level::INFO:
                    level_str = "[INFO]   ";
                    break;
                case log_level::DEBUG:
                    level_str = "[DEBUG]  ";
                    break;
                case log_level::WARN:
                    level_str = "[WARN]   ";
                    break;
                case log_level::ERROR:
                    level_str = "[ERROR]  ";
                    break;
                default:
                    level_str = "[UNKNOWN] ";
                    break;
            }
            return level_str;
        }

        std::string get_curr_time()
        {
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf = log_time_util::localtime_safe(t);

            char buf[TIME_BUF_SIZE];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
            return std::string(buf);
        }

        template<typename ... Args>
        std::string format_msg(const std::string& format, Args&&... args)
        {
            std::vector<std::string> arg_strs = {to_str_helper(std::forward<Args>(args))...};
            std::ostringstream oss; // res
            size_t arg_idx = 0;
            size_t pos = 0;
            size_t placeholder = format.find("{}", pos);
            int n = arg_strs.size();

            while (placeholder != std::string::npos)
            {
                oss << format.substr(pos, placeholder - pos);
                if (arg_idx < n) oss << arg_strs[arg_idx ++ ];
                else oss << "{}";
                pos = placeholder + 2;
                placeholder = format.find("{}", pos);
            }

            oss << format.substr(pos);
            while (arg_idx < n)
                oss << arg_strs[arg_idx ++ ];

            return "[" + get_curr_time() + "] " + oss.str();
        }

};

#endif // LOGGER_DEMO_H
