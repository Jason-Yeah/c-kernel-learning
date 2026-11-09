#pragma once
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <utility>

[[noreturn]] inline void system_fail(const char *what)
{
    throw std::system_error(errno, std::generic_category(), what);
}

class UniqueFd
{
    int fd_ = -1;

public:
    explicit UniqueFd(int fd = -1) noexcept : fd_(fd) {}
    ~UniqueFd() { reset(); }
    UniqueFd(const UniqueFd &) = delete;
    UniqueFd &operator=(const UniqueFd &) = delete;
    UniqueFd(UniqueFd &&other) noexcept : fd_(std::exchange(other.fd_, -1)) {}
    void reset() noexcept
    {
        if (fd_ >= 0)
            ::close(std::exchange(fd_, -1));
    }
    int get() const noexcept { return fd_; }
};

// 只用于本目录单线程、会自行结束的短子进程；析构等待并回收，可能阻塞。
class Child
{
    pid_t pid_;

public:
    explicit Child(pid_t pid) noexcept : pid_(pid) {}
    Child(const Child &) = delete;
    Child &operator=(const Child &) = delete;
    ~Child()
    {
        if (pid_ > 0)
        {
            while (::waitpid(pid_, nullptr, 0) < 0 && errno == EINTR) {}
        }
    }
    int wait()
    {
        if (pid_ <= 0)
            throw std::logic_error("child already reaped");
        int status;
        while (::waitpid(pid_, &status, 0) < 0)
        {
            if (errno != EINTR)
                system_fail("waitpid");
        }
        pid_ = -1;
        return status;
    }
};
