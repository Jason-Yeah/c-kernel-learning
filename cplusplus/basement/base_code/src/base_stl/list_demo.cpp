#include "list_demo.h"

/* test main */
int main()
{
    /* 
    std::list<int> ilist;

    ilist.push_back(1);
    ilist.push_back(2);
    ilist.push_back(3);
    ilist.push_front(0);

    for (auto it = ilist.begin(); it != ilist.end(); ++it)
    {
        std::cout << *it << " ";
    }
    std::cout << std::endl;

    auto it_begin = ilist.begin();
    it_begin++;
    ilist.insert(it_begin, 10);

    ilist.remove(2);

    for (auto it = ilist.begin(); it != ilist.end(); ++it)
    {
        std::cout << *it << " ";
    }
    std::cout << std::endl;
    */

    sen_stl::list<int> my_list;
    my_list.push_back(1);
    my_list.push_back(2);
    my_list.push_back(3);
    my_list.push_front(0);

    auto it = my_list.begin();
    it ++ ;
    my_list.insert(it, 10);
    my_list.insert(it, 20);
    my_list.insert(it, 20);

    my_list.print();

    my_list.remove(20);
    my_list.print();
    
    return 0;
}