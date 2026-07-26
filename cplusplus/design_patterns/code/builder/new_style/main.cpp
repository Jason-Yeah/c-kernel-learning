#include "builder.hpp"

int main()
{
    auto house = HouseConfigBuilder()
                     .SetFloors(2)
                     .SetRooms(5)
                     .SetGarages(1)
                     .WithGarden()
                     .WithPool()
                     .SetColor("white")
                     .Build();

    house.ShowConfig();
    return 0;
}