#include <stdio.h>

struct Node {
    int id;           // 4字节
    char tag;         // 1字节
    // 这里会有 3字节 的 Padding (对齐)
    int *data_ptr;    // 8字节指针
};

int mystery_recursive(int n) {
    if (n <= 1) return 1;
    return n + mystery_recursive(n - 1); // 简单的递归累加
}

int main() {
    int my_array[3] = {0x11223344, 0x55667788, 0x99AABBCC};
    
    struct Node node;
    node.id = 0xDEADC0DE;
    node.tag = 'A';    // ASCII 0x41
    node.data_ptr = &my_array[1]; // 指向数组的第二个元素 (0x55667788)

    int result = mystery_recursive(3); 
    
    printf("Result: %d\n", result); // Result: 6;
    return 0;
}