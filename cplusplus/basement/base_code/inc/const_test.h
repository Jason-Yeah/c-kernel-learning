#include <iostream>

#ifndef CONST_TEST_H
#define CONST_TEST_H

const int val = 10; // 不推荐
extern const int gval2; // 推荐

extern void print_addr();

#endif