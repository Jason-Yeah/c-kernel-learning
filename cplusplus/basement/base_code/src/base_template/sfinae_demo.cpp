#include "sfinae_demo.h"

int main()
{
    int x = 10;
    print_type(x);

    double y = 2.1;
    print_type(y);

    const char* s = "hello world";
    print_type(s);

    // char arr[] = "char array";
    // print_type(arr);

    print_type(&x);

    WithFoo wf;
    // WithoutFoo wof;

    call_foo(wf);
    // call_foo(wof);

    std::vector<int> v;

    TypePrinter<WithValueType>::print();
    TypePrinter<WithoutValueType>::print();


    return 0;
}
