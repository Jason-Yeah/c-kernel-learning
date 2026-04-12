#include <iostream>

namespace aspace
{
    int aid = 857;
}

namespace bspace
{
    int bid = 798;
}

int main()
{
    std::cout << "A: " << aspace::aid << std::endl;

    std::cout << "B: " << bspace::bid << std::endl;

    return 0;
}
