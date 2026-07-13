#pragma once

#include <iostream>

template <typename T> class singleton
{
public:
    singleton(const singleton &) = delete;
    singleton &operator=(const singleton &) = delete;

    static T &instance()
    {
        static T inst;
        return inst;
    }

protected:
    singleton() = default;
    ~singleton() = default;
};
