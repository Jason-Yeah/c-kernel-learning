#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

class NewHandlerGuard
{
public:
    explicit NewHandlerGuard(std::new_handler handler) noexcept
        : oldHandler_(std::set_new_handler(handler))
    {
    }

    // RAII
    ~NewHandlerGuard() { std::set_new_handler(oldHandler_); }

    NewHandlerGuard(const NewHandlerGuard &) = delete;
    NewHandlerGuard &operator=(const NewHandlerGuard &) = delete;

private:
    std::new_handler oldHandler_;
};

class Widget
{
public:
    static std::new_handler setNewHandler(std::new_handler handler) noexcept
    {
        const std::new_handler old = currentHandler_;
        currentHandler_ = handler;
        return old;
    }

    static void failNextAllocationForDemo() noexcept
    {
        failNextAllocation_ = true;
    }

    static void allowAllocationAfterHandler() noexcept
    {
        failNextAllocation_ = false;
    }

    static void *operator new(std::size_t size)
    {
        // 在调用全局分配函数期间，暂时启用 Widget 专属 handler。
        NewHandlerGuard guard{currentHandler_};

        // 为了让实验确定可复现，failNextAllocation_ 会跳过第一次 malloc，
        // 模拟底层失败。handler 返回后，循环会再次尝试分配。
        if (size == 0)
        {
            size = 1;
        }

        while (true)
        {
            if (!failNextAllocation_)
            {
                if (void *memory = std::malloc(size))
                {
                    return memory;
                }
            }

            const std::new_handler handler = std::get_new_handler();
            if (handler == nullptr)
            {
                throw std::bad_alloc{};
            }
            handler();
        }
    }

    static void operator delete(void *memory) noexcept { std::free(memory); }

private:
    inline static std::new_handler currentHandler_ = nullptr;
    inline static bool failNextAllocation_ = false;
    int value_{};
};

void widgetOutOfMemory()
{
    // 真正 OOM handler 应避免再次动态分配。这里用 C stdio 输出固定文本。
    std::fputs("Widget handler: release emergency resource\n", stderr);
    Widget::allowAllocationAfterHandler();
}

void globalHandler()
{
    std::fputs("global handler\n", stderr);
    throw std::bad_alloc{};
}

int main()
{
    const std::new_handler originalGlobalHandler =
        std::set_new_handler(globalHandler);

    Widget::setNewHandler(widgetOutOfMemory);
    Widget::failNextAllocationForDemo();

    Widget *widget = new Widget;
    delete widget;

    // Widget::operator new 离开后，RAII guard 已恢复原来的全局 handler。
    std::cout << std::boolalpha << "global handler restored: "
              << (std::get_new_handler() == globalHandler) << '\n';

    std::set_new_handler(originalGlobalHandler);
}

/*
new
operator new
malloc / allocator
glibc heap allocator

brk mmap
kernel
*/