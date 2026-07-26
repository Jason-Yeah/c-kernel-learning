#pragma once

#include "house.hpp"
#include <iostream>
#include <memory>
#include <string>

class HouseBuilder
{
public:
    virtual ~HouseBuilder() = default;

    virtual void BuildFoundation() = 0;

    virtual void BuildWalls() = 0;

    virtual void BuildRoof() = 0;

    virtual void BuildInterior() = 0;

    virtual std::unique_ptr<House> GetResult() = 0;
};

class WoodenHouseBuilder : public HouseBuilder
{
public:
    WoodenHouseBuilder() : house_(std::make_unique<House>()) {}

    void BuildFoundation() override { house_->SetFoundation("木桩地基"); }

    void BuildWalls() override { house_->SetWalls("原木墙体"); }

    void BuildRoof() override { house_->SetRoof("木瓦屋顶"); }

    void BuildInterior() override { house_->SetInterior("简装修"); }

    std::unique_ptr<House> GetResult() override { return std::move(house_); }

private:
    std::unique_ptr<House> house_;
};

class BrickHouseBuilder : public HouseBuilder
{
    std::unique_ptr<House> house_;

public:
    BrickHouseBuilder() : house_(std::make_unique<House>()) {}

    void BuildFoundation() override { house_->SetFoundation("钢筋混凝土地基"); }

    void BuildWalls() override { house_->SetWalls("红砖墙体"); }

    void BuildRoof() override { house_->SetRoof("琉璃瓦屋顶"); }

    void BuildInterior() override { house_->SetInterior("精装修"); }

    std::unique_ptr<House> GetResult() override { return std::move(house_); }
};
