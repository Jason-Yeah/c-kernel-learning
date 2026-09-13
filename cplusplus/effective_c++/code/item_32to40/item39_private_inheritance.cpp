#include <cassert>
#include <iostream>
#include <type_traits>

class Timer
{
public:
    virtual ~Timer() = default;
    void tick() { onTick(); }

private:
    virtual void onTick() = 0;
};

class Monitor final : private Timer
{
public:
    void poll() { tick(); } // 内部借用 Timer 的流程。
    int samples() const { return samples_; }

private:
    void onTick() override { ++samples_; }
    int samples_{};
};

struct EmptyPolicy
{
};

struct WithMember
{
    EmptyPolicy policy;
    int value{};
};

struct WithBase : private EmptyPolicy
{
    int value{};
};

#if __cplusplus >= 202002L
struct WithAttribute
{
    [[no_unique_address]] EmptyPolicy policy;
    int value{};
};
#endif

int main()
{
    Monitor monitor;
    monitor.poll();
    monitor.poll();
    std::cout << "samples=" << monitor.samples() << '\n';
    assert(monitor.samples() == 2);
    static_assert(!std::is_convertible_v<Monitor *, Timer *>);
    // Timer* timer = &monitor; // 外部不能经 private 继承进行转换。
    std::cout << "sizeof empty/member/base: " << sizeof(EmptyPolicy) << '/'
              << sizeof(WithMember) << '/' << sizeof(WithBase) << '\n';
#if __cplusplus >= 202002L
    std::cout << "sizeof no_unique_address member: " << sizeof(WithAttribute)
              << '\n';
#endif
}
