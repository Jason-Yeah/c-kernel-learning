# 0 "hello.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "hello.c"
# 1 "./my_print.h" 1



void print_str(char* str);
# 2 "hello.c" 2

int main()
{

    char *str = "Hello World\n";

    print_str(str);

    return 0;
}
