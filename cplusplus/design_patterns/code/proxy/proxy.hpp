#pragma once

#include <iostream>
#include <memory>
#include <mutex>

class Counter
{
public:
    virtual ~Counter() = default;
    virtual void inc() = 0;
    virtual int getValue() = 0;
};

class UnsafeCounter : public Counter
{
public:
    void inc() override
    {
        int t = val_;
        ++t;
        val_ = t;
    }

    int getValue() override { return val_; }

private:
    int val_ = 0;
};

class SafeCounterProxy : public Counter
{
public:
    explicit SafeCounterProxy(std::unique_ptr<Counter> real)
        : real_(std::move(real))
    {
    }

    void inc() override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        real_->inc();
    }

    int getValue() override
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return real_->getValue();
    }

private:
    std::unique_ptr<Counter> real_;
    std::mutex mtx_;
};
