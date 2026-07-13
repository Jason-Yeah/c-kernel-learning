#pragma once

template <typename T>
class singleton
{
public:
    singleton(const singleton &) = delete;
    singleton &operator=(const singleton &) = delete;
    singleton(singleton &&) = delete;
    singleton &operator=(singleton &&) = delete;

    static T &instance()
    {
        static T instance;
        return instance;
    }

protected:
    singleton() = default;
    ~singleton() = default;
};
