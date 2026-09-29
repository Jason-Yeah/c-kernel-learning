#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <cerrno>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <syslog.h>
#include <system_error>
#include <unistd.h>
#include <utility>

class syslog_session
{
public:
    explicit syslog_session(const char *ident)
    {
        ::openlog(ident, LOG_PID | LOG_NDELAY, LOG_USER);
    }
    ~syslog_session() { ::closelog(); }
    syslog_session(const syslog_session &) = delete;
    syslog_session &operator=(const syslog_session &) = delete;
};
int main(int argc, char **)
try
{
    if (argc != 1)
        throw std::runtime_error("usage: server_standards_cpp20");
    syslog_session log("server_standards_cpp20");
    ::syslog(LOG_INFO, "C++20 process=%ld cwd=%s", (long)::getpid(),
             std::filesystem::current_path().c_str());
    std::cout << "日志已发送到系统日志服务；当前目录："
              << std::filesystem::current_path().string() << '\n';
}
catch (const std::filesystem::filesystem_error &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
