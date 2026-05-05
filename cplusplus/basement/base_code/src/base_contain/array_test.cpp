#include <iostream>
#include <vector>

char * my_strcpy(char* dest, const char* src)
{
    char* res_d = dest;
    while (*src != '\0')
    {
        *dest = *src;
        dest ++ ;
        src ++ ;
    }
    *dest = '\0';
    return res_d;
}

int main()
{
    int arr[10];

    printf("%ld\n", sizeof(arr));
    printf("%ld\n", sizeof(arr) / sizeof(int));

    std::cout << "first element: address is " << &arr[0] << std::endl;
    // 数组首地址
    std::cout << "arr address is " << arr << std::endl;
    // 数组首地址
    std::cout << "arr address is " << &arr << std::endl;


    {
        int j = 2;
        const int i = 42;             // i 是顶层 const
        const int *p = &i;            // p 是底层 const（指向常量的指针）
        int * const p2 = &j;

        auto a0 = p2;
        auto a1 = i;                  // a1 是 int（顶层 const 被忽略）
        auto a2 = p;                  // a2 是 const int*（底层 const 被保留！）
    
        decltype(i) d1 = 10;          // d1 是 const int（保留了顶层 const）
        decltype(p) d2 = p;           // d2 是 const int*（保留了底层 const）
    }

    // ia3是一个含有10个整数的数组
    decltype(arr) ia3 = {0,1,2,3,4,5,6,7,8,9};

    int ia[] = {1, 2, 3, 4, 555};
    int *iabeg = std::begin(ia);
    int *iaend = std::end(ia);

    for (auto it = iabeg; it != iaend; it ++ )
        std::cout << *it << " ";
    std::cout << std::endl;

    //计算数组元素个数
    auto n = std::end(ia) - std::begin(ia);
    std::cout << "n is " << n << std::endl;
    std::cout << "last element is " << *(std::begin(ia) + n - 1) << std::endl;
    
    std::vector<int> vec(std::begin(ia), std::end(ia));
    
    for (auto t: vec)
        printf("%d ", t);
    puts("");

    const char* source = "Hello, World!";
    char dest[50];

    printf("%s\n", dest);

    my_strcpy(dest, source);

    printf("%s\n", dest);

    return 0;
}