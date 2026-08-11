#include "itlist.hpp"
#include <iostream>

int main()
{
    MyList<int> nums;
    nums.push_back(10);
    nums.push_back(20);
    nums.push_back(30);

    auto it = nums.create_iterator();
    for (it->first(); !it->is_done(); it->next())
        std::cout << it->current_item() << " ";

    std::cout << std::endl;

    return 0;
}