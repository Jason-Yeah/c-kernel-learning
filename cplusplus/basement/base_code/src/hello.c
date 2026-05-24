#include <stdio.h>

__attribute__((constructor)) void hello ()
{
    printf("hello\n");
}

__attribute__((destructor)) void bye ()
{
    printf("bye main\n");
}

int main() {}
