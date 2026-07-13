// singleton.hpp — 单例模板基类
// 任何类想成为单例，只需继承 Singleton<T>，然后调用 T::get_instance()
#pragma once

#include <memory>

template <typename T> class Singleton
{
public:
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;
    Singleton(Singleton &&) = delete;
    Singleton &operator=(Singleton &&) = delete;

    // 返回 T&，防止误拷贝。
    // C++11 起局部 static 变量初始化是线程安全的。
    static T &get_instance()
    {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    // 析构不设 virtual——没人应该通过基类指针 delete 单例。
    // 单例在程序结束时自动销毁。
    ~Singleton() = default;
};
