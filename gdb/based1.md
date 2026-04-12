# PWNDBG

## 整体布局

```bash
pwndbg> b main
Breakpoint 1 at 0x1151: file hello_test.c, line 7.
pwndbg> r
Starting program: /home/jason/gdb/src/hello_test
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".

Breakpoint 1, main () at hello_test.c:7
7           i ++ ;
LEGEND: STACK | HEAP | CODE | DATA | WX | RODATA
────────────────────────────────[ LAST SIGNAL ]─────────────────────────────────
Breakpoint hit at 0x555555555151
─────────────[ REGISTERS / show-flags off / show-compact-regs off ]─────────────
 RAX  0x555555555149 (main) ◂— endbr64
 RBX  0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
 RDX  0x7fffffffdd48 —▸ 0x7fffffffe05b ◂— 'SHELL=/bin/bash'
 RDI  1
 RSI  0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
 R8   0
 R9   0x7ffff7fca380 (_dl_fini) ◂— endbr64
 R10  0x7fffffffd930 ◂— 0x800000
 R11  0x203
 R12  1
 R13  0
 R14  0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
 R15  0x7ffff7ffd000 (_rtld_global) —▸ 0x7ffff7ffe2e0 —▸ 0x555555554000 ◂— 0x10102464c457f
 RBP  0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
 RSP  0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
 RIP  0x555555555151 (main+8) ◂— mov eax, dword ptr [rip + 0x2eb9]
──────────────────────[ DISASM / x86-64 / set emulate on ]──────────────────────
b► 0x555555555151 <main+8>     mov    eax, dword ptr [rip + 0x2eb9]     EAX, [i] => 1
   0x555555555157 <main+14>    add    eax, 1                            EAX => 2 (1 + 1)
   0x55555555515a <main+17>    mov    dword ptr [rip + 0x2eb0], eax     [i] <= 2
   0x555555555160 <main+23>    mov    eax, dword ptr [rip + 0x2eaa]     EAX, [i] => 2
   0x555555555166 <main+29>    mov    esi, eax                          ESI => 2
   0x555555555168 <main+31>    lea    rax, [rip + 0xe95]                RAX => 0x555555556004 ◂— 'Hello, World! %d\n'
   0x55555555516f <main+38>    mov    rdi, rax                          RDI => 0x555555556004 ◂— 'Hello, World! %d\n'
   0x555555555172 <main+41>    mov    eax, 0                            EAX => 0
   0x555555555177 <main+46>    call   printf@plt                  <printf@plt>

   0x55555555517c <main+51>    mov    eax, 0                 EAX => 0
   0x555555555181 <main+56>    pop    rbp
───────────────────────────────[ SOURCE (CODE) ]────────────────────────────────
In file: /home/jason/gdb/src/hello_test.c:7
    1 #include <stdio.h>
    2
    3 int i = 1;
    4
    5 int main()
    6 {
 ►  7     i ++ ;
    8     printf("Hello, World! %d\n", i);
    9
   10     return 0;
   11 }
───────────────────────────────────[ STACK ]────────────────────────────────────
00:0000│ rbp rsp 0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
01:0008│+008     0x7fffffffdc18 —▸ 0x7ffff7c2a1ca (__libc_start_call_main+122) ◂— mov edi, eax
02:0010│+010     0x7fffffffdc20 —▸ 0x7fffffffdc60 —▸ 0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
03:0018│+018     0x7fffffffdc28 —▸ 0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
04:0020│+020     0x7fffffffdc30 ◂— 0x155554040
05:0028│+028     0x7fffffffdc38 —▸ 0x555555555149 (main) ◂— endbr64
06:0030│+030     0x7fffffffdc40 —▸ 0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
07:0038│+038     0x7fffffffdc48 ◂— 0xf341dc1f64ce732e
─────────────────────────────────[ BACKTRACE ]──────────────────────────────────
 ► 0   0x555555555151 main+8
   1   0x7ffff7c2a1ca __libc_start_call_main+122
   2   0x7ffff7c2a28b __libc_start_main+139
   3   0x555555555085 _start+37
────────────────────────────────────────────────────────────────────────────────
pwndbg>
```

## 关注点

### REGISTERS寄存器组

1. 最重要：RIP，实际就是PC。

    ```bash
    RIP  0x555555555151 (main+8) ◂— mov eax, dword ptr [rip + 0x2eb9]
    ```

2. RAX累加寄存器：现在是main的地址，后期i++运算中将会作为临时中转站

    ```bash
    RAX  0x555555555149 (main) ◂— endbr64
    ```

3. RSP/RBP：指向栈，当然还相等，说明Main刚开始，还没在栈上分配空间

### DISASM（反汇编）

`►`代表即将执行那行。pwbdbg已经预测了结果`EAX, [i]=>1`，意思是执行这行i会赋值为1存入EAX中

注意` mov    eax, dword ptr [rip + 0x2eb9]     EAX, [i] => 1`，ptr是Pointer缩写，告诉CPU要操作的数据有多大。

内存是一个连续字节序列，当写`[rip + 0x2eb9]`时，只是给了CPU一个起始地址，但CPU不知道从这个地址开始读几个字节，`dword`就是4字节也就是int。

- byte 1B
- word 2B
- dword 4B
- qword 8B

在GDB中对于MOV指令永远是`MOV S, D`，也就是S=D

### SOURCE源码

在编译时通过`-g`选项把汇编与源程序关联在一起，此时马上要处理的是第7行

### STACK

这里特殊因为i是全局变量，不在栈上在DATA段中，栈上主要都是返回地址

### BACKTRACE调用栈回溯

程序如何一步步运行到当前位置，从下到上的栈。

最起点是`_start`是程序绝对入口；`__lib_start_main`是C语言标准库glibc的启动代码，在main运行钱，负责做一些初始化准备工作；最顶层`main`表示当前程序暂停在main函数第八个字节指令处。

## ni后变化

```bash
pwndbg> ni
0x0000555555555157      7           i ++ ;
LEGEND: STACK | HEAP | CODE | DATA | WX | RODATA
 RBX  0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
 RCX  0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
 RDX  0x7fffffffdd48 —▸ 0x7fffffffe05b ◂— 'SHELL=/bin/bash'
 RDI  1
 RSI  0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
 R8   0
 R9   0x7ffff7fca380 (_dl_fini) ◂— endbr64
 R10  0x7fffffffd930 ◂— 0x800000
 R11  0x203
 R12  1
 R13  0
 R14  0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
 R15  0x7ffff7ffd000 (_rtld_global) —▸ 0x7ffff7ffe2e0 —▸ 0x555555554000 ◂— 0x10102464c457f
 RBP  0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
 RSP  0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
*RIP  0x555555555157 (main+14) ◂— add eax, 1
──────────────────────[ DISASM / x86-64 / set emulate on ]──────────────────────
b+ 0x555555555151 <main+8>     mov    eax, dword ptr [rip + 0x2eb9]     EAX, [i] => 1
 ► 0x555555555157 <main+14>    add    eax, 1                            EAX => 2 (1 + 1)
   0x55555555515a <main+17>    mov    dword ptr [rip + 0x2eb0], eax     [i] <= 2
   0x555555555160 <main+23>    mov    eax, dword ptr [rip + 0x2eaa]     EAX, [i] => 2
   0x555555555166 <main+29>    mov    esi, eax                          ESI => 2
   0x555555555168 <main+31>    lea    rax, [rip + 0xe95]                RAX => 0x555555556004 ◂— 'Hello, World! %d\n'
   0x55555555516f <main+38>    mov    rdi, rax                          RDI => 0x555555556004 ◂— 'Hello, World! %d\n'
   0x555555555172 <main+41>    mov    eax, 0                            EAX => 0
   0x555555555177 <main+46>    call   printf@plt                  <printf@plt>

   0x55555555517c <main+51>    mov    eax, 0                 EAX => 0
   0x555555555181 <main+56>    pop    rbp
───────────────────────────────[ SOURCE (CODE) ]────────────────────────────────
In file: /home/jason/gdb/src/hello_test.c:7
    1 #include <stdio.h>
    2
    3 int i = 1;
    4
    5 int main()
    6 {
 ►  7     i ++ ;
    8     printf("Hello, World! %d\n", i);
    9
   10     return 0;
   11 }
───────────────────────────────────[ STACK ]────────────────────────────────────
00:0000│ rbp rsp 0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
01:0008│+008     0x7fffffffdc18 —▸ 0x7ffff7c2a1ca (__libc_start_call_main+122) ◂— mov edi, eax
02:0010│+010     0x7fffffffdc20 —▸ 0x7fffffffdc60 —▸ 0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
03:0018│+018     0x7fffffffdc28 —▸ 0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
04:0020│+020     0x7fffffffdc30 ◂— 0x155554040
05:0028│+028     0x7fffffffdc38 —▸ 0x555555555149 (main) ◂— endbr64
06:0030│+030     0x7fffffffdc40 —▸ 0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
07:0038│+038     0x7fffffffdc48 ◂— 0xf341dc1f64ce732e
─────────────────────────────────[ BACKTRACE ]──────────────────────────────────
 ► 0   0x555555555157 main+14
   1   0x7ffff7c2a1ca __libc_start_call_main+122
   2   0x7ffff7c2a28b __libc_start_main+139
   3   0x555555555085 _start+37
```

REGISTERS：RIP是下一条指令没问题；此时通过`info r`看到RAX寄存器当前内容为1

当执行到`lea rax, [rip + 0xe95]`实际上是用Lea指令算字符串常量的绝对地址。可以通过`pwndbg> x/s 0x555555556004`查看内存中字符串的内容

- x：查看内存
- /s： 以string格式显示
- /16cb：显示16c字符，按字节b显示

```bash
pwndbg> x/16cb 0x555555556004
0x555555556004: 72 'H'  101 'e' 108 'l' 108 'l' 111 'o' 44 ','  32 ' '  87 'W'
0x55555555600c: 111 'o' 114 'r' 108 'l' 100 'd' 33 '!'  32 ' '  37 '%'  100 'd'
```

而在Linux的ELF格式中，字符串存放在.rodata段中大概就是

```text
.text 代码指令
.rodata 字符串常量
.data 初始化全局变量
```

使用`vmmap`可以看vm布局，如：

```bash
pwndbg> vmmap
LEGEND: STACK | HEAP | CODE | DATA | WX | RODATA
             Start                End Perm     Size  Offset File (set vmmap-prefer-relpaths on)
    0x555555554000     0x555555555000 r--p     1000       0 hello_test
    0x555555555000     0x555555556000 r-xp     1000    1000 hello_test
    0x555555556000     0x555555557000 r--p     1000    2000 hello_test
    0x555555557000     0x555555558000 r--p     1000    2000 hello_test
    0x555555558000     0x555555559000 rw-p     1000    3000 hello_test
    0x7ffff7c00000     0x7ffff7c28000 r--p    28000       0 /usr/lib/x86_64-linux-gnu/libc.so.6
    0x7ffff7c28000     0x7ffff7db0000 r-xp   188000   28000 /usr/lib/x86_64-linux-gnu/libc.so.6
    0x7ffff7db0000     0x7ffff7dff000 r--p    4f000  1b0000 /usr/lib/x86_64-linux-gnu/libc.so.6
    0x7ffff7dff000     0x7ffff7e03000 r--p     4000  1fe000 /usr/lib/x86_64-linux-gnu/libc.so.6
    0x7ffff7e03000     0x7ffff7e05000 rw-p     2000  202000 /usr/lib/x86_64-linux-gnu/libc.so.6
    0x7ffff7e05000     0x7ffff7e12000 rw-p     d000       0 [anon_7ffff7e05]
    0x7ffff7fb4000     0x7ffff7fb7000 rw-p     3000       0 [anon_7ffff7fb4]
    0x7ffff7fbd000     0x7ffff7fbf000 rw-p     2000       0 [anon_7ffff7fbd]
    0x7ffff7fbf000     0x7ffff7fc3000 r--p     4000       0 [vvar]
    0x7ffff7fc3000     0x7ffff7fc5000 r-xp     2000       0 [vdso]
    0x7ffff7fc5000     0x7ffff7fc6000 r--p     1000       0 /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x7ffff7fc6000     0x7ffff7ff1000 r-xp    2b000    1000 /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x7ffff7ff1000     0x7ffff7ffb000 r--p     a000   2c000 /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x7ffff7ffb000     0x7ffff7ffd000 r--p     2000   36000 /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x7ffff7ffd000     0x7ffff7fff000 rw-p     2000   38000 /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x7ffffffde000     0x7ffffffff000 rw-p    21000       0 [stack]
```

由于字符串常量是不可变的，所以一定是在一个read-only区间内，观察0x555555556004所在的区间。

`0x555555556000     0x555555557000 r--p     1000    2000 hello_test`

可以看到这部分是只有r权限。

这个布局反应了：

```bash
起始地址,权限 (Perm),对应 ELF 段,包含什么内容？
0x555555554000, r--p, .interp 等, 只读元数据。
0x555555555000, r-xp, .text, 你的代码指令（可读、可执行、不可改）。
0x555555556000, r--p, .rodata," 你的字符串常数（比如 ""Hello, World! %d\n""）。"
0x555555558000, rw-p, .data / .bss, 全局变量 i（可读、可写）。
```

对于共享库区域：0x7ffff7开头

```bash
0x7ffff7c28000     0x7ffff7db0000 r-xp   188000   28000 /usr/lib/x86_64-linux-gnu/libc.so.6
```

`libc.so.6`标准C库。当前printf函数代码就在这个r--xp的段内。

`ld-linux-x86-64.so.2`动态链接器，负责程序启动时把printf真实地址填写到GOT表中。

比如现在要看全局变量i的内容，那肯定要知道它地址是什么

```bash
pwndbg> p &i
$2 = (int *) 0x555555558010 <i>
```

## 追踪printf函数执行过程

```bash
pwndbg> si
0x0000555555555050 in printf@plt ()
LEGEND: STACK | HEAP | CODE | DATA | WX | RODATA
─────────────[ REGISTERS / show-flags off / show-compact-regs off ]─────────────
 RAX  0
 RBX  0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
 RCX  0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
 RDX  0x7fffffffdd48 —▸ 0x7fffffffe05b ◂— 'SHELL=/bin/bash'
 RDI  0x555555556004 ◂— 'Hello, World! %d\n'
 RSI  2
 R8   0
 R9   0x7ffff7fca380 (_dl_fini) ◂— endbr64
 R10  0x7fffffffd930 ◂— 0x800000
 R11  0x203
 R12  1
 R13  0
 R14  0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
 R15  0x7ffff7ffd000 (_rtld_global) —▸ 0x7ffff7ffe2e0 —▸ 0x555555554000 ◂— 0x10102464c457f
 RBP  0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
*RSP  0x7fffffffdc08 —▸ 0x55555555517c (main+51) ◂— mov eax, 0
*RIP  0x555555555050 (printf@plt) ◂— endbr64
──────────────────────[ DISASM / x86-64 / set emulate on ]──────────────────────
 ► 0x555555555050 <printf@plt>      endbr64
   0x555555555054 <printf@plt+4>    jmp    qword ptr [rip + 0x2f76]    <printf>
    ↓
   0x7ffff7c60100 <printf>          endbr64
   0x7ffff7c60104 <printf+4>        push   rbp
   0x7ffff7c60105 <printf+5>        mov    rbp, rsp                        RBP => 0x7fffffffdc00 —▸ 0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— ...
   0x7ffff7c60108 <printf+8>        sub    rsp, 0xd0                       RSP => 0x7fffffffdb30 (0x7fffffffdc00 - 0xd0)
   0x7ffff7c6010f <printf+15>       mov    qword ptr [rbp - 0xa8], rsi     [0x7fffffffdb58] <= 2
   0x7ffff7c60116 <printf+22>       mov    qword ptr [rbp - 0xa0], rdx     [0x7fffffffdb60] <= 0x7fffffffdd48 —▸ 0x7fffffffe05b ◂— 'SHELL=/bin/bash'
   0x7ffff7c6011d <printf+29>       mov    qword ptr [rbp - 0x98], rcx     [0x7fffffffdb68] <= 0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
   0x7ffff7c60124 <printf+36>       mov    qword ptr [rbp - 0x90], r8      [0x7fffffffdb70] <= 0
   0x7ffff7c6012b <printf+43>       mov    qword ptr [rbp - 0x88], r9      [0x7fffffffdb78] <= 0x7ffff7fca380 (_dl_fini) ◂— endbr64
───────────────────────────────────[ STACK ]────────────────────────────────────
00:0000│ rsp 0x7fffffffdc08 —▸ 0x55555555517c (main+51) ◂— mov eax, 0
01:0008│ rbp 0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
02:0010│+008 0x7fffffffdc18 —▸ 0x7ffff7c2a1ca (__libc_start_call_main+122) ◂— mov edi, eax
03:0018│+010 0x7fffffffdc20 —▸ 0x7fffffffdc60 —▸ 0x555555557dc0 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555100 (__do_global_dtors_aux) ◂— endbr64
04:0020│+018 0x7fffffffdc28 —▸ 0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
05:0028│+020 0x7fffffffdc30 ◂— 0x155554040
06:0030│+028 0x7fffffffdc38 —▸ 0x555555555149 (main) ◂— endbr64
07:0038│+030 0x7fffffffdc40 —▸ 0x7fffffffdd38 —▸ 0x7fffffffe03c ◂— '/home/jason/gdb/src/hello_test'
─────────────────────────────────[ BACKTRACE ]──────────────────────────────────
 ► 0   0x555555555050 printf@plt
   1   0x55555555517c main+51
   2   0x7ffff7c2a1ca __libc_start_call_main+122
   3   0x7ffff7c2a28b __libc_start_main+139
   4   0x555555555085 _start+37
```

### 栈变化

当前栈顶部

```bash
00:0000│ rsp 0x7fffffffdc08 —▸ 0x55555555517c (main+51) ◂— mov eax, 0
01:0008│ rbp 0x7fffffffdc10 —▸ 0x7fffffffdcb0 —▸ 0x7fffffffdd10 ◂— 0
```

当执行call指令时，CPU会把call的下一条指令地址也就是main+51压入栈中，把RIP修改为`printf@plt`地址，此时RSP会减小8B（栈向下增长），存的是返回地址。

RSP是0x7fffffffdc08内容是0x55555555517c，是栈顶，存的是返回地址0x55555555517c，返回后下一条指令是`mov eax, 0`

下一条指令是`jmp    qword ptr [rip + 0x2f76]    <printf>`这时代码是在找printf在内存哪里，需要通过GOT找，`[rip+0x2f76]`就是对应的GOT的一个槽位。

发现在执行jmp指令后，RIP会跳转到`0x7ffff7..`区域，标志已经进入了libc.so.6的地方，也就是printf

### 金丝雀

```bash
0x7ffff7c60156 <printf+86>     mov    rax, qword ptr fs:[0x28]            RAX, [0x7ffff7fb4768] => 0xe4936c744ff0af00
```

执行后RAX从0变成了`*RAX  0xe4936c744ff0af00`。为什么会有：printf这种内部逻辑复杂，为防止在处理字符串时发生缓冲区溢出攻击，在函数开始时从`fs:[0x28]`（一个受保护的系统位置）取出了一个随机数存在栈底，在函数返回前会检查这个位置值有没有变，如果变直接崩溃报错，不是让攻击者去执行恶意代码。

`FS`是一个段寄存器，核心作用是作为基地址，指向一块特殊的内存区域，Linux利用FS寄存器特性，将其固定指向当前线程的TLS头部，`fs:[0x28]`就是当前线程私有数据区中，专门存放“栈保护随机数”的位置。

验证：

```bash
\pwndbg> fsbase
0x7ffff7fb4740
pwndbg> x/gx 0x7ffff7fb4740 + 0x28
0x7ffff7fb4768: 0xe4936c744ff0af00
```

### 返回值

```bash
Run till exit from #0  0x00007ffff7c60173 in __printf (
    format=0x555555556004 "Hello, World! %d\n") at ./stdio-common/printf.c:33
Hello, World! 2
main () at hello_test.c:10
10          return 0;
Value returned is $3 = 16
```

执行完printf后会返回一个`$3 = 16`在GDB中，RAX存放的是函数返回值，printf返回值是成功打印的字符数，此时RAX寄存器的内容为`RAX  0x10`