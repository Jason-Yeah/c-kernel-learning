#pragma once
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <stdexcept>

// C++11 有界队列；数据、closed 标记与等待谓词使用同一个 mutex。
class BoundedQueue
{
    std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::queue<int> values_;
    std::size_t capacity_;
    bool closed_ = false;

public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity)
    {
        if (capacity == 0)
            throw std::invalid_argument("queue capacity must be positive");
    }
    BoundedQueue(const BoundedQueue &) = delete;
    BoundedQueue &operator=(const BoundedQueue &) = delete;

    bool push(int value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this] { return closed_ || values_.size() < capacity_; });
        if (closed_)
            return false;
        values_.push(value);
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    bool pop(int &value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return closed_ || !values_.empty(); });
        if (values_.empty())
            return false; // 关闭且已排空。
        value = values_.front();
        values_.pop();
        lock.unlock();
        not_full_.notify_one();
        return true;
    }

    void close()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }
};
