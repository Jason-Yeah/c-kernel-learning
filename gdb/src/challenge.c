#include <stdio.h>
#include <string.h>

struct User {
    char username[8];
    int level;         // 用户等级
    int *auth_code_ptr; // 指向验证码的指针
};

int calculate_key(int n) {
    if (n <= 1) return 5;
    return n * 2 + calculate_key(n - 1);
}

void login() {
    int secret_code = 2026;
    struct User jason;
    
    // 初始化
    strcpy(jason.username, "admin");
    jason.level = 1;
    jason.auth_code_ptr = &secret_code;

    // 核心挑战点
    int key = calculate_key(3); 
    
    if (key == 17 && jason.level > 100) {
        printf("--- ACCESS GRANTED: WELCOME MASTER ---\n");
    } else {
        printf("Access Denied. Key: %d, Level: %d\n", key, jason.level);
    }
}

int main() {
    login();
    return 0;
}