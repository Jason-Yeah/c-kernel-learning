#pragma once
#include <coroutine>
#include <exception>
#include <utility>

// 教学用 void 协程所有者：lazy start、显式恢复、在最终挂起点保留帧。
// 不提供 operator co_await，也不自带线程、调度器或取消策略。
class Task
{
public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        std::exception_ptr error;

        Task get_return_object()
        {
            return Task{Handle::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    explicit Task(Handle handle) noexcept : handle_(handle) {}
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    Task(Task &&other) noexcept : handle_(std::exchange(other.handle_, {})) {}
    Task &operator=(Task &&other) noexcept
    {
        if (this != &other)
        {
            if (handle_)
                handle_.destroy();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    ~Task()
    {
        if (handle_)
            handle_.destroy();
    }

    bool done() const noexcept { return !handle_ || handle_.done(); }

    void resume()
    {
        if (!done())
            handle_.resume();
        rethrow_if_failed();
    }

    void rethrow_if_failed() const
    {
        if (handle_ && handle_.promise().error)
            std::rethrow_exception(handle_.promise().error);
    }

private:
    Handle handle_{};
};
