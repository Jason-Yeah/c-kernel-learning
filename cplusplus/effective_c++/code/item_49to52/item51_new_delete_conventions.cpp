#include <cstddef>
#include <cstdint>
#include <iostream>
#include <new>

class PoolCandidate
{
public:
    virtual ~PoolCandidate() = default;

    static void *operator new(std::size_t size)
    {
        if (size != sizeof(PoolCandidate))
        {
            std::cout << "derived size " << size
                      << ": delegate to global operator new\n";
            return ::operator new(size);
        }

        std::cout << "base-sized request " << size
                  << ": a real implementation could use its fixed pool\n";
        return ::operator new(size);
    }

    static void operator delete(void *memory) noexcept
    {
        ::operator delete(memory);
    }

private:
    int value_{};
};

class DerivedCandidate : public PoolCandidate
{
private:
    int extra_[100]{};
};

class alignas(64) CacheLineObject
{
public:
    static void *operator new(std::size_t size)
    {
        return ::operator new(size, std::align_val_t{alignof(CacheLineObject)});
    }

    static void operator delete(void *memory) noexcept
    {
        ::operator delete(memory, std::align_val_t{alignof(CacheLineObject)});
    }

private:
    int value_{};
};

int main()
{
    PoolCandidate *base = new PoolCandidate;
    delete base;

    // 继承来的类专属 operator new 会收到 sizeof(DerivedCandidate)。
    DerivedCandidate *derived = new DerivedCandidate;
    delete derived;

    CacheLineObject *cacheObject = new CacheLineObject;
    const auto address = reinterpret_cast<std::uintptr_t>(cacheObject);
    std::cout << "64-byte aligned: " << std::boolalpha
              << (address % alignof(CacheLineObject) == 0) << '\n';
    delete cacheObject;
}
