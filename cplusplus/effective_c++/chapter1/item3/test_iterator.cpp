#include <iostream>
#include <vector>

int main()
{
    std::vector<int> vec = {1, 2, 3, 4, 5};

    const std::vector<int>::iterator iter = vec.begin();
    std::cout << *iter << std::endl; // Output: 1
    *iter = 10;
    // iter ++ ; // error: increment of read-only iterator 'iter'
    std::cout << *iter << std::endl; // Output: 10
    std::cout << *(iter + 2) << std::endl; // Output: 3

    std::vector<int>::const_iterator cIter = vec.begin();

    // *cIter = 20; // error: assignment of read-only location '* cIter'
    ++ cIter;
    std::cout << *cIter << std::endl; // Output: 2

    return 0;
}