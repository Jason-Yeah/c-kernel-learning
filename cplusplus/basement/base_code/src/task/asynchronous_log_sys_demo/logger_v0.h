
#ifndef LOGGER_DEMO_H
#define LOGGER_DEMO_H

#include <chrono>
#include <cstddef>
#include <ctime>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <stdexcept>
#include <utility>
#include <vector>

#define TIME_BUF_SIZE 50

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
        // void push(const std::string& msg)
        // {
        //     std::lock_guard<std::mutex> lock(_mtx);
        //     _queue.push(msg);
        //     if (_queue.size() == 1) _cond_var.notify_one();
        // }
        bool push(const std::string& msg)
        {
            {
                std::lock_guard<std::mutex> lock(_mtx);
                if (_is_shutdown) return false;
                
                _queue.push(std::move(msg));

                if (_queue.size() > 0)
                    _cond_var.notify_one();
            }
            return true;
        }

        bool pop(std::string& msg)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            // if (_queue.empty())
            //     _cond_var.wait(lock);
            // 表示队列非空或线程要关闭了就不用一直挂起了
            // wait会 1.释放Lock所持有互斥锁 2.线程加入_cond_var等待队列
            // _is_shut_down为true wait返回往下执行，如果_queue为空返回false不为空取余下数据
            // is_shut_down为false 如果队列为空，wait不返回线程挂起等待，如果队列非空wait返回，往下取数据
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

class logger
{
    public:
        logger(const std::string& filename):_log_file(filename, std::ios::app |
                     std::ios::out), _exit_flag(false)
        {
            if (!_log_file.is_open()) throw std::runtime_error("Failed to open log file");
            
            _worker_thread = std::thread([this]{
                std::string msg;
                while (_lqueue.pop(msg))
                    _log_file << msg << std::endl;
            });

            _log_file.flush();
        } 

        ~logger()
        {
            _exit_flag = true;
            _lqueue.shutdown();
            if (_worker_thread.joinable())  _worker_thread.join();

            if (_log_file.is_open()) _log_file.close();
        }

        // 转发引用
        template<typename ... Args>
        void log(log_level level, const std::string& format, Args&&... args)
        {
            std::string level_str;
            switch (level) 
            {
                case log_level::INFO:
                    level_str = "[INFO]  ";
                    break;
                case log_level::DEBUG:
                    level_str = "[DEBUG] ";
                    break;
                case log_level::WARN:
                    level_str = "[WARN]  ";
                    break;
                case log_level::ERROR:
                    level_str = "[ERROR] ";
                    break;
            }
            _lqueue.push(level_str + format_msg(format, std::forward<Args>(args)...));
        }
    private:
        log_queue _lqueue;
        std::thread _worker_thread;
        std::ofstream _log_file;
        std::atomic<bool> _exit_flag;

        std::string get_curr_time()
        {
            auto now = std::chrono::system_clock::now(); // obj: time_point
            std::time_t time = std::chrono::system_clock::to_time_t(now); // CType: second
            // return std::ctime(&time); // to string
            char buf[TIME_BUF_SIZE];
            
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time));
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
