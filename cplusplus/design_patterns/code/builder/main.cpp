
#include "director.hpp"
#include <iostream>

int main()
{
    Director director;
    WoodenHouseBuilder wood;
    auto wood_house = director.Construct(wood);
    wood_house->Show();

    BrickHouseBuilder brick;
    auto brick_house = director.Construct(brick);
    brick_house->Show();

    return 0;
}