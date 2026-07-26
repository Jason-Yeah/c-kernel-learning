#pragma once

#include "builder.hpp"

class Director
{
public:
    std::unique_ptr<House> Construct(HouseBuilder &builder)
    {
        builder.BuildFoundation(); // 步骤1：建地基（必须最先）
        builder.BuildWalls();      // 步骤2：建墙体
        builder.BuildRoof();       // 步骤3：盖屋顶
        builder.BuildInterior();   // 步骤4：装修
        return builder.GetResult();
    }
};
