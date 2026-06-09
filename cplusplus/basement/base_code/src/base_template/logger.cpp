#include "logger.h"
#include <string>



int main()
{
    int a = 10;
    sen_std::logger<int>::log(a);

    int* intp = &a;
    sen_std::logger<int *>::log(intp);

    std::string str = "JASONYE";
    sen_std::logger<std::string>::log(str);
    
    sen_std::log_all(a, intp, str, nullptr);

    std::cout << "--------------------------------\n";

    std::cout << sen_std::factorial<5>::val << std::endl;
    std::cout << &sen_std::factorial<5>::val << std::endl;

    std::cout << sen_std::fibonacci<10>::val << std::endl;

    static_assert(sen_std::is_addable<int>::value, "int should be addable");
    static_assert(!sen_std::is_addable<void*>::value, "void* should be addable");

    std::cout << sen_std::sum<1, 2, 3, 4, 6>::val << std::endl;

    using list = sen_std::type_list<int, double, char>;
    using third_type = sen_std::type_at<list, 2>::type;
    std::cout << typeid(third_type).name() << std::endl;

    return 0;
}