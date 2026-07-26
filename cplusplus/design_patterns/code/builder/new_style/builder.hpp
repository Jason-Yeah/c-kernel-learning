#pragma once

#include "house.hpp"
#include <string>

// ============ 流式建造者 ============
class HouseConfigBuilder
{
    House::Config config_;

public:
    // ★ 每个 setter 返回 *this，实现链式调用
    HouseConfigBuilder &SetFloors(int n)
    {
        config_.floors = n;
        return *this;
    }

    HouseConfigBuilder &SetRooms(int n)
    {
        config_.rooms = n;
        return *this;
    }

    HouseConfigBuilder &SetGarages(int n)
    {
        config_.garages = n;
        return *this;
    }

    HouseConfigBuilder &SetColor(const std::string &c)
    {
        config_.color = c;
        return *this;
    }

    HouseConfigBuilder &SetMaterial(const std::string &m)
    {
        config_.material = m;
        return *this;
    }

    HouseConfigBuilder &WithGarden()
    {
        config_.hasGarden = true;
        return *this;
    }
    
    HouseConfigBuilder &WithPool()
    {
        config_.hasPool = true;
        return *this;
    }

    House Build()
    {
        House h;
        h.config_ = config_;
        return h;
    }
};
