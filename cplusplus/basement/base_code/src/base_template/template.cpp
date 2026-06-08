#include "template.h"
#include <iostream>
#include <ostream>

int main()
{
    std::cout << max_val(2, 3) << std::endl;
    std::cout << max_val(2.1, 3.9) << std::endl;
    std::cout << max_val("abc", "def") << std::endl; // 字符串字面量const char*，比成内存地址大小了
    std::cout << max_val<std::string>("abc", "def") << std::endl; 
    
    sen_std::pair<int, double> p1(1, 2.5);
    p1.print();

    sen_std::fixed_array<int, 10> f1;
    for (size_t i = 0; i < 10; ++ i)
        f1[i] = i + 1;
    f1.print();

    sen_std::container_printer<std::vector, int> vec_printer;
    std::vector<int> vec = {1, 2, 3, 4, 5};

    vec_printer.print(vec);

    sen_std::printer<int> ip;
    ip.print(20);

    sen_std::printer<std::string> sp;
    sp.print("shit");

    double val = 4.5;
    int ival = 10;
    int* pi = &ival;
    sen_std::pair<std::string, double*> p2("abc", &val);
    p2.print();    

    sen_std::printval(10);
    sen_std::printval(std::string("1111"));
    sen_std::printval(pi);
    
    sen_std::printall(10, 20.5, "Hello");
    sen_std::coutall(10, 20.5, "Hello");

    std::cout << sen_std::sum_all(10, 2, 3.14, 233) << std::endl;
    std::cout << sen_std::all_not(false, false, false) << std::endl;

    std::cout << sen_std::sum_all(1, 2, 3, 4, 5) << std::endl;
    std::cout << sen_std::multi_right_fold(1, 2, 3, 4, 5) << std::endl;
    sen_std::print_all('a', 'b', 'c', 'd');

    sen_std::my_point mp1{1, 2}, mp2{3, 4}, mp3{5, 6};
    auto res = sen_std::sum_point(mp1, mp2, mp3);

    std::cout << res.x << " " << res.y << std::endl;

    return 0;
}