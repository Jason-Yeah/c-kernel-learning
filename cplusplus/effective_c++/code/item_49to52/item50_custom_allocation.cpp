#include <array>
#include <cstddef>
#include <iostream>
#include <memory_resource>
#include <new>
#include <vector>

class TrackedMessage
{
public:
    explicit TrackedMessage(int id) : id_(id) {}

    static void *operator new(std::size_t size)
    {
        ++allocations_;
        bytesRequested_ += size;
        return ::operator new(size);
    }

    static void operator delete(void *memory) noexcept
    {
        ++deallocations_;
        ::operator delete(memory);
    }

    static void printStatistics()
    {
        std::cout << "TrackedMessage allocations/deallocations/bytes: "
                  << allocations_ << '/' << deallocations_ << '/'
                  << bytesRequested_ << '\n';
    }

    int id() const { return id_; }

private:
    inline static std::size_t allocations_ = 0;
    inline static std::size_t deallocations_ = 0;
    inline static std::size_t bytesRequested_ = 0;
    int id_;
};

int main()
{
    TrackedMessage *message = new TrackedMessage{7};
    std::cout << "message id: " << message->id() << '\n';
    delete message;
    TrackedMessage::printStatistics();

    // 现代做法：只把特定容器的分配导向明确的 memory_resource，
    // 不改变整个进程的全局 operator new。
    std::array<std::byte, 1024> localBuffer{};
    std::pmr::monotonic_buffer_resource arena{localBuffer.data(),
                                              localBuffer.size(),
                                              std::pmr::null_memory_resource()};
    std::pmr::vector<int> values{&arena};

    for (int value = 1; value <= 5; ++value)
    {
        values.push_back(value * 10);
    }

    std::cout << "pmr values:";
    for (const int value : values)
    {
        std::cout << ' ' << value;
    }
    std::cout << '\n';
}
