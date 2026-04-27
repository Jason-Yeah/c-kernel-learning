#include <iostream>

typedef double wages;
typedef wages base, *pwage;
typedef char* pstring;

using i64_t = long long;

int main()
{
    wages wage = 3500;
    base bwage = wage;
    pwage pw = &wage;
    i64_t a = 10;

    const pstring str = 0;
    *str = 'm';

    {
        const int a = 10, &ra = a;
        decltype(a) x = 0;
        decltype(ra) y = x;
        
        int i = 10, *p = &i;
        decltype(*p) pi = i;
        decltype((i)) f = i;
    }

    return 0;
}