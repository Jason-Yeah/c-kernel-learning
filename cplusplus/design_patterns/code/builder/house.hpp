#pragma once

#include <iostream>
#include <string>

class House
{
public:
    struct Detail
    {
        std::string foundation;
        std::string walls;
        std::string roof;
        std::string interior;
    };

    void SetFoundation(const std::string &f) { detail_.foundation = f; }

    void SetWalls(const std::string &w) { detail_.walls = w; }

    void SetRoof(const std::string &r) { detail_.roof = r; }

    void SetInterior(const std::string &i) { detail_.interior = i; }

    void Show() const
    {
        std::cout << "  地基: " << detail_.foundation << "\n"
                  << "  墙体: " << detail_.walls << "\n"
                  << "  屋顶: " << detail_.roof << "\n"
                  << "  装修: " << detail_.interior << std::endl;
    }

private:
    Detail detail_;
};
