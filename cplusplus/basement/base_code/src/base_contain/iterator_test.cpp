#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

void change_vec(std::vector<int> & vec)
{
    // for (auto it = vec.begin(); it != vec.end();)
    // {
    //     if (*it % 2) 
    //     {
    //         it = vec.erase(it);
    //         continue;
    //     }
    //     it ++ ;
    // }

    auto new_end = std::remove_if(vec.begin(), vec.end(), [](int x){
        return x % 2;
    });

    vec.erase(new_end, vec.end());
}

int main()
{
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::vector<int>::iterator it;
    std::vector<int>::const_iterator cit;

    for (cit = vec.cbegin(); cit != vec.cend(); cit ++ )
        std::cout << *cit << " ";
    puts("");

    std::vector<std::string> text = {"aaa", "", "ccc"};
    for (auto it = text.cbegin(); it != text.cend() && !it->empty(); it ++ )
        std::cout << *it << std::endl;

    change_vec(vec);
    for (auto it = vec.cbegin(); it != vec.cend(); it ++ )
        std::cout << *it << " ";
    puts("");

    return 0;
}