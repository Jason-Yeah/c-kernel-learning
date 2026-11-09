#ifndef OS_DEMO_H
#define OS_DEMO_H
#include <stdint.h>
#define OS_LEFT 3
#define OS_RIGHT 2
extern int os_data;
extern int os_bss;
extern const char os_message[];
int os_add(int left, int right);
uint64_t os_width(void);
int os_equal(int left, int right);
#endif
