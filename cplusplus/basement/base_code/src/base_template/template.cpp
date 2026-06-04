#include "template.h"

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

    return 0;
}