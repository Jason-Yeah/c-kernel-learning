#include "map_demo.h"

int main()
{
    /*
    std::map<int, std::string> my_map = {
        {1, "one"},
        {3, "three"},
        {2, "two"}        
    };

    my_map.insert({4, "four"});
    my_map.insert(std::pair<int, std::string>(5, "five"));
    my_map.insert(std::make_pair(6, "six"));
    my_map[7] = "seven"; // 先查找7是否存在，如果不存在则插入7并赋值为"seven"，如果存在则直接赋值为"seven"

    for (auto it = my_map.begin(); it != my_map.end(); ++ it)
        std::cout << it->first << ": " << it->second << std::endl;

    auto num = my_map[4];
    std::cout << "num: " << num << std::endl;

    auto it = my_map.find(8);
    if (it != my_map.end())
        std::cout << "find 8: " << it->second << std::endl;
    else
        std::cout << "not find 8" << std::endl;

    // std::out_of_range exception
    try
    {
        num = my_map.at(8);
    }
    catch(const std::out_of_range& e)
    {
        std::cerr << "Key not found: " <<  e.what() << '\n';
    }

    my_map.erase(3);
    for (auto& t: my_map)
    {
        std::cout << t.first << ": " << t.second << std::endl;
    }

    auto it_st = my_map.find(2);
    auto it_ed = my_map.find(5);

    // for (auto it = it_st; it != it_ed; ++ it)
    //     my_map.erase(it);
    
    my_map.erase(it_st, it_ed);
    std::cout << "1=============\n";
    for (auto& t: my_map)
    {
        std::cout << t.first << ": " << t.second << std::endl;
    }

    std::cout << "2=============\n";
    // my_map.clear();
    // for (auto& t: my_map)
    // {
    //     std::cout << t.first << ": " << t.second << std::endl;
    // }

    it_st = my_map.lower_bound(1);
    it_ed = my_map.upper_bound(6);

    std::cout << it_st->first << std::endl;
    std::cout << it_ed->first << std::endl;

    // 左闭右开
    std::for_each(it_st, it_ed, [](auto pair){
        std::cout << pair.first << ": " << pair.second << std::endl;
    });
    */
    
    sen_std::bst_map::map<int, std::string> my_map;
    my_map.insert({1, "one"});
    my_map.insert(std::make_pair(2, "two"));

    return 0;
}