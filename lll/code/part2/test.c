#include<stdio.h>

int main()
{
    #ifdef __cplusplus
    printf("c++\n");
    #else
    printf("C\n");
    #endif

    return 0;
}