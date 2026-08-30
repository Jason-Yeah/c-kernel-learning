#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

class Character
{
public:
    virtual ~Character() = default;

    virtual void Render(int x, int y, int fontSize) const = 0;

    virtual char GetChar() const = 0;
};

class CharFlyWeight : public Character
{
    char ch_;

public:
    explicit CharFlyWeight(char ch) : ch_(ch) {}

    void Render(int x, int y, int fontSize) const override
    {
        std::cout << "  渲染字符 '" << ch_ << "' @(" << x << "," << y
                  << ") 字号 " << fontSize << " [对象地址: " << this << "]"
                  << std::endl;
    }

    char GetChar() const override { return ch_; }
};

class CharFactory
{
    std::unordered_map<char, std::unique_ptr<Character>> pool_;

public:
    Character *GetFlyWeight(char ch)
    {
        auto it = pool_.find(ch);
        if (it != pool_.end())
        {
            std::cout << "  [工厂] 复用已有字符 '" << ch << "'" << std::endl;
            return it->second.get(); // 复用
        }

        std::cout << "  [工厂] 新建字符 '" << ch << "'" << std::endl;
        auto fly = std::make_unique<CharFlyWeight>(ch);
        auto *raw = fly.get();
        pool_[ch] = std::move(fly);
        return raw;
    }

    size_t PoolSize() const { return pool_.size(); }
};
