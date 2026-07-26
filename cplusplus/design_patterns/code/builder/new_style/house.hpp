#pragma once

#include <iostream>
#include <string>

class House
{
public:
    struct Config
    {
        int floors = 1;
        int rooms = 3;
        int garages = 1;
        std::string color = "white";
        std::string material = "brick";
        bool hasGarden = false;
        bool hasPool = false;
    };

    void ShowConfig() const
    {
        std::cout << "  层数: " << config_.floors << "\n"
                  << "  房间: " << config_.rooms << "\n"
                  << "  车库: " << config_.garages << "\n"
                  << "  颜色: " << config_.color << "\n"
                  << "  材料: " << config_.material << "\n"
                  << "  花园: " << (config_.hasGarden ? "有" : "无") << "\n"
                  << "  泳池: " << (config_.hasPool ? "有" : "无") << std::endl;
    }

private:
    Config config_;

    friend class HouseConfigBuilder;
};
