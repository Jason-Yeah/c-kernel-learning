#include "deque_demo.h"

int main()
{
    
    sen_std::deque<int> d;
    d.push_back(1);
    d.push_back(2);
    d.push_front(0);

    d.pop_back();
    d.pop_front();

    for (auto it = d.begin(); it!= d.end(); ++ it)
    {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    

    return 0;
}