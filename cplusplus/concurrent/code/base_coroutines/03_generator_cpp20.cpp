#include <coroutine>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

// int 生成器：每次 next 恢复到下一个 co_yield；这里只实现教学所需接口。
class IntGenerator
{
public:
    struct promise_type;
    using Handle = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        int current = 0;
        std::exception_ptr error;
        IntGenerator get_return_object() { return IntGenerator{Handle::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int n) noexcept
        {
            current = n;
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    explicit IntGenerator(Handle h) noexcept : handle_(h) {}
    IntGenerator(const IntGenerator &) = delete;
    IntGenerator &operator=(const IntGenerator &) = delete;
    IntGenerator(IntGenerator &&other) noexcept
        : handle_(std::exchange(other.handle_, {})),
          available_(std::exchange(other.available_, false)) {}
    ~IntGenerator()
    {
        if (handle_)
            handle_.destroy();
    }

    bool next()
    {
        available_ = false;
        if (!handle_ || handle_.done())
            return false;
        handle_.resume();
        if (handle_.promise().error)
            std::rethrow_exception(handle_.promise().error);
        available_ = !handle_.done();
        return available_;
    }

    int value() const
    {
        if (!handle_ || !available_)
            throw std::logic_error("call next() successfully before value()");
        return handle_.promise().current;
    }

private:
    Handle handle_{};
    bool available_ = false;
};

IntGenerator numbers(int count)
{
    for (int n = 1; n <= count; ++n)
        co_yield n; // 产出一个整数，暂停，等待消费者 next()。
    co_return;
}

int main()
try
{
    auto sequence = numbers(5);
    while (sequence.next())
        std::cout << sequence.value() << '\n';
}
catch (const std::exception &e)
{
    std::cerr << e.what() << '\n';
    return 1;
}
