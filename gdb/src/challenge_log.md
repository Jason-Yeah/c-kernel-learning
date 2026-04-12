# 练习记录

```bash
gcc -Og -g challenge.c -o challenge

# ./challenge : Access Denied. Key: 15, Level: 1
```

## TASK

任务 A：内存“大搜查” (Structure Layout)
在 login 函数执行到 int key = ... 这一行时停下。

找到结构体 jason 在栈上的起始地址。

硬核挑战：不要用 p jason.level，请直接查看内存（x/gx 或 stack 窗口），找出 level 这个成员变量在内存中的十六进制原始数据。

观察点：看看 username 和 level 之间是否存在你之前提到的 Padding（内存对齐填充）？

任务 B：递归“拦截器” (Return Value Manipulation)
程序逻辑中 key == 17 才能通过。

跟进 calculate_key(3)。

硬核挑战：观察 calculate_key 返回给 login 时的 EAX。你会发现默认算出来不是 17。

操作：在 calculate_key 准备 ret 回到 login 的瞬间，手动用 GDB 把 EAX 修改掉，让 key 变成 17。

任务 C：内存“手术刀” (Value Injection)
即便 key 对了，jason.level 默认只有 1，还是进不去。

在 if 判断执行前，找到 jason.level 的内存地址。

硬核挑战：使用 set {int}地址 = 1337 命令，直接在物理内存上修改这个值。

---

## 初始状态

现在选择在main函数打断点

```bash
LEGEND: STACK | HEAP | CODE | DATA | WX | RODATA
────────────────────────────────[ LAST SIGNAL ]─────────────────────────────────
Breakpoint hit at 0x55555555520a
─────────────[ REGISTERS / show-flags off / show-compact-regs off ]─────────────
 RAX  0x55555555520a (main) ◂— endbr64
 RBX  0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
 RDX  0x7fffffffdd58 —▸ 0x7fffffffe05f ◂— 'SHELL=/bin/bash'
 RDI  1
 RSI  0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
 R8   0
 R9   0x7ffff7fca380 (_dl_fini) ◂— endbr64
 R10  0x7fffffffd940 ◂— 0x800000
 R11  0x203
 R12  1
 R13  0
 R14  0x555555557db8 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555120 (__do_global_dtors_aux) ◂— endbr64
 R15  0x7ffff7ffd000 (_rtld_global) —▸ 0x7ffff7ffe2e0 —▸ 0x555555554000 ◂— 0x10102464c457f
 RBP  0x7fffffffdcc0 —▸ 0x7fffffffdd20 ◂— 0
 RSP  0x7fffffffdc28 —▸ 0x7ffff7c2a1ca (__libc_start_call_main+122) ◂— mov edi, eax
 RIP  0x55555555520a (main) ◂— endbr64
──────────────────────[ DISASM / x86-64 / set emulate on ]──────────────────────
b► 0x55555555520a <main>       endbr64
   0x55555555520e <main+4>     sub    rsp, 8     RSP => 0x7fffffffdc20 (0x7fffffffdc28 - 0x8)
   0x555555555212 <main+8>     mov    eax, 0     EAX => 0
   0x555555555217 <main+13>    call   login                       <login>

   0x55555555521c <main+18>    mov    eax, 0                 EAX => 0
   0x555555555221 <main+23>    add    rsp, 8
   0x555555555225 <main+27>    ret

   0x555555555226              add    byte ptr [rax], al
   0x555555555228 <_fini>      endbr64
   0x55555555522c <_fini+4>    sub    rsp, 8
   0x555555555230 <_fini+8>    add    rsp, 8
───────────────────────────────[ SOURCE (CODE) ]────────────────────────────────
In file: /home/jason/gdb/src/challenge.c:34
   24     // 核心挑战点
   25     int key = calculate_key(3);
   26
   27     if (key == 17 && jason.level > 100) {
   28         printf("--- ACCESS GRANTED: WELCOME MASTER ---\n");
   29     } else {
   30         printf("Access Denied. Key: %d, Level: %d\n", key, jason.level);
   31     }
   32 }
   33
 ► 34 int main() {
   35     login();
   36     return 0;
   37 }
───────────────────────────────────[ STACK ]────────────────────────────────────
00:0000│ rsp 0x7fffffffdc28 —▸ 0x7ffff7c2a1ca (__libc_start_call_main+122) ◂— mov edi, eax
01:0008│-090 0x7fffffffdc30 —▸ 0x7fffffffdc70 —▸ 0x555555557db8 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555120 (__do_global_dtors_aux) ◂— endbr64
02:0010│-088 0x7fffffffdc38 —▸ 0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
03:0018│-080 0x7fffffffdc40 ◂— 0x155554040
04:0020│-078 0x7fffffffdc48 —▸ 0x55555555520a (main) ◂— endbr64
05:0028│-070 0x7fffffffdc50 —▸ 0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
06:0030│-068 0x7fffffffdc58 ◂— 0x6b447a4d45309a7e
07:0038│-060 0x7fffffffdc60 ◂— 1
─────────────────────────────────[ BACKTRACE ]──────────────────────────────────
 ► 0   0x55555555520a main
   1   0x7ffff7c2a1ca __libc_start_call_main+122
   2   0x7ffff7c2a28b __libc_start_main+139
   3   0x5555555550a5 _start+37
```

观察初态：

```bash
   0x55555555520e <main+4>     sub    rsp, 8     RSP => 0x7fffffffdc20 (0x7fffffffdc28 - 0x8)
 ► 0x555555555212 <main+8>     mov    eax, 0     EAX => 0
   0x555555555217 <main+13>    call   login                       <login>
```

首先sub rsp, 8为了内存对齐（当要call一个函数的时候，rsp必须是16的整数倍），然后mov eax, 0对应的是main中return 0（编译器优化习惯，暂时不重要）。

## TASK_A

### 进入login函数

```bash
──────────────────────[ DISASM / x86-64 / set emulate on ]──────────────────────
 ► 0x555555555188 <login>       endbr64
   0x55555555518c <login+4>     sub    rsp, 0x38                              RSP => 0x7fffffffdbe0 (0x7fffffffdc18 - 0x38)
   0x555555555190 <login+8>     mov    rax, qword ptr fs:[0x28]               RAX, [0x7ffff7fb4768] => 0xe5466c53fc847d00
   0x555555555199 <login+17>    mov    qword ptr [rsp + 0x28], rax            [0x7fffffffdc08] <= 0xe5466c53fc847d00
   0x55555555519e <login+22>    xor    eax, eax                               EAX => 0
   0x5555555551a0 <login+24>    mov    dword ptr [rsp + 0xc], 0x7ea           [{secret_code}] <= 0x7ea
   0x5555555551a8 <login+32>    mov    dword ptr [rsp + 0x10], 0x696d6461     [{jason}] <= 0x696d6461
   0x5555555551b0 <login+40>    mov    word ptr [rsp + 0x14], 0x6e            [{jason+0x4}] <= 0x6e
   0x5555555551b7 <login+47>    mov    dword ptr [rsp + 0x18], 1              [{jason+0x8}] <= 1
   0x5555555551bf <login+55>    lea    rax, [rsp + 0xc]                       RAX => 0x7fffffffdbec ◂— 0x696d6461000007ea
   0x5555555551c4 <login+60>    mov    qword ptr [rsp + 0x20], rax            [{jason+0x10}] <= 0x7fffffffdbec ◂— 0x696d6461000007ea
───────────────────────────────[ SOURCE (CODE) ]────────────────────────────────
In file: /home/jason/gdb/src/challenge.c:15
    9
   10 int calculate_key(int n) {
   11     if (n <= 1) return 5;
   12     return n * 2 + calculate_key(n - 1);
   13 }
   14
 ► 15 void login() {
   16     int secret_code = 2026;
   17     struct User jason;
   18
   19     // 初始化
   20     strcpy(jason.username, "admin");
   21     jason.level = 1;
   22     jason.auth_code_ptr = &secret_code;
───────────────────────────────────[ STACK ]────────────────────────────────────
00:0000│ rsp 0x7fffffffdc18 —▸ 0x55555555521c (main+18) ◂— mov eax, 0
01:0008│-0a0 0x7fffffffdc20 —▸ 0x7fffffffdcc0 —▸ 0x7fffffffdd20 ◂— 0
02:0010│-098 0x7fffffffdc28 —▸ 0x7ffff7c2a1ca (__libc_start_call_main+122) ◂— mov edi, eax
03:0018│-090 0x7fffffffdc30 —▸ 0x7fffffffdc70 —▸ 0x555555557db8 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555120 (__do_global_dtors_aux) ◂— endbr64
04:0020│-088 0x7fffffffdc38 —▸ 0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
05:0028│-080 0x7fffffffdc40 ◂— 0x155554040
06:0030│-078 0x7fffffffdc48 —▸ 0x55555555520a (main) ◂— endbr64
07:0038│-070 0x7fffffffdc50 —▸ 0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
─────────────────────────────────[ BACKTRACE ]──────────────────────────────────
 ► 0   0x555555555188 login
   1   0x55555555521c main+18
   2   0x7ffff7c2a1ca __libc_start_call_main+122
   3   0x7ffff7c2a28b __libc_start_main+139
   4   0x5555555550a5 _start+37
```

可以看到

```bash
0x555555555188 <login>       endbr64
0x55555555518c <login+4>     sub    rsp, 0x38                              RSP => 0x7fffffffdbe0 (0x7fffffffdc18 - 0x38)
0x555555555190 <login+8>     mov    rax, qword ptr fs:[0x28]               RAX, [0x7ffff7fb4768] => 0xe5466c53fc847d00
0x555555555199 <login+17>    mov    qword ptr [rsp + 0x28], rax            [0x7fffffffdc08] <= 0xe5466c53fc847d00
0x55555555519e <login+22>    xor    eax, eax                               EAX => 0
```

首先sub了56B，我们观察结构体

```c
struct User {
    char username[8];
    int level;         // 用户等级
    int *auth_code_ptr; // 指向验证码的指针
};
```

$8 + 4 + 4(padding) + 8 = 24B$，还有其他两个int变量（secret_code, key），还有个Canary是8B，现在是40B，符合rsp是16B的倍数，剩下16B被用来作为对齐和缓冲区保护。

接下来赋值`0x5555555551a0 <login+24>    mov    dword ptr [rsp + 0xc], 0x7ea           [{secret_code}] <= 0x7ea`这里十六进制数对应十进制就是2026。

#### __strcpy_chk

当前编译器开启了优化，不再用之前的strcpy，而是替换了现在__strcpy_chk。是GLIBC防止缓冲区溢出。

编译器发现拷贝的是一个固定的常量字符串 "admin"，调用 strcpy 太慢了，干脆直接用两条 mov 指令把这 5 个字母硬编码写进了栈里。

```bash
 ► 0x5555555551a8 <login+32>    mov    dword ptr [rsp + 0x10], 0x696d6461     [{jason}] <= 0x696d6461 # 'admi'
   0x5555555551b0 <login+40>    mov    word ptr [rsp + 0x14], 0x6e            [{jason+0x4}] <= 0x6e # 'n'
```

对于`jason.auth_code_ptr = &secret_code;`赋值在底层实际上是

```bash
 ► 0x5555555551bf <login+55>    lea    rax, [rsp + 0xc]                       RAX => 0x7fffffffdbec ◂— 0x696d6461000007ea
   0x5555555551c4 <login+60>    mov    qword ptr [rsp + 0x20], rax            [{jason+0x10}] <= 0x7fffffffdbec ◂— 0x696d6461000007ea
```

首先lea计算地址，把rsp+0xc结果放入rax中。然后把rax中的内容压栈。而之前`mov    dword ptr [rsp + 0xc], 0x7ea`就是2026，也就是rsp+0xc就是secret_code的地址。

#### 递归前栈情况

在进入递归函数前分析栈的情况。

```bash
00:0000│ rsp   0x7fffffffdbe0 ◂— 0
01:0008│ rax-4 0x7fffffffdbe8 ◂— 0x7ea00000000
02:0010│-0d0   0x7fffffffdbf0 {jason} ◂— 0x6e696d6461 /* 'admin' */
03:0018│-0c8   0x7fffffffdbf8 {jason+0x8} ◂— 1
04:0020│-0c0   0x7fffffffdc00 {jason+0x10} —▸ 0x7fffffffdbec {secret_code} ◂— 0x696d6461000007ea
05:0028│-0b8   0x7fffffffdc08 ◂— 0xd1d141296e12fc00
06:0030│-0b0   0x7fffffffdc10 —▸ 0x7fffffffdd00 —▸ 0x555555555080 (_start) ◂— endbr64
07:0038│-0a8   0x7fffffffdc18 —▸ 0x55555555521c (main+18) ◂— mov eax, 0
```

可以清晰看出，从栈上看，离栈顶最近的是0x7ea00000000，也就是局部变量2026，然后0x6e696d6461是'jason'，小端存储因为6e实际是n，然后是0x7fffffffdc00，这个位置存的是0x7fffffffdbec，是引用也就是rsp+0xc。

对于struct的padding问题也显然可以看出：首先username是char[8]占8B，从0x7fffffffdbf0开始到0x7fffffffdbf8这是8B，然后level是从dbf8到dc00，中间是8B，但level实际是4B，说明这其中填充了4B的padding。

## TASK_B

进入递归函数后

```bash
─────────────[ REGISTERS / show-flags off / show-compact-regs off ]─────────────
 RAX  0x7fffffffdbec ◂— 0x696d6461000007ea
 RBX  0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
 RCX  0x555555557db8 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555120 (__do_global_dtors_aux) ◂— endbr64
 RSI  0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
 R8   0
 R9   0x7ffff7fca380 (_dl_fini) ◂— endbr64
 R10  0x7fffffffd940 ◂— 0x800000
 R11  0x203
 R12  1
 R13  0
 R14  0x555555557db8 (__do_global_dtors_aux_fini_array_entry) —▸ 0x555555555120 (__do_global_dtors_aux) ◂— endbr64
 R15  0x7ffff7ffd000 (_rtld_global) —▸ 0x7ffff7ffe2e0 —▸ 0x555555554000 ◂— 0x10102464c457f
 RBP  0x7fffffffdcc0 —▸ 0x7fffffffdd20 ◂— 0
*RSP  0x7fffffffdbd8 —▸ 0x5555555551d3 (login+75) ◂— mov edx, eax
*RIP  0x555555555169 (calculate_key) ◂— endbr64
──────────────────────[ DISASM / x86-64 / set emulate on ]──────────────────────
 ► 0x555555555169 <calculate_key>       endbr64
   0x55555555516d <calculate_key+4>     cmp    edi, 1     3 - 1     EFLAGS => 0x202 [ cf pf af zf sf IF df of iopl:00 ac ]
   0x555555555170 <calculate_key+7>   ✘ jle    calculate_key+25            <calculate_key+25>

   0x555555555172 <calculate_key+9>     push   rbx
   0x555555555173 <calculate_key+10>    lea    ebx, [rdi + rdi]     EBX => 6
   0x555555555176 <calculate_key+13>    sub    edi, 1               EDI => 2 (3 - 1)
   0x555555555179 <calculate_key+16>    call   calculate_key               <calculate_key>

   0x55555555517e <calculate_key+21>    add    eax, ebx
   0x555555555180 <calculate_key+23>    pop    rbx
   0x555555555181 <calculate_key+24>    ret

   0x555555555182 <calculate_key+25>    mov    eax, 5               EAX => 5
───────────────────────────────[ SOURCE (CODE) ]────────────────────────────────
In file: /home/jason/gdb/src/challenge.c:10
    4 struct User {
    5     char username[8];
    6     int level;         // 用户等级
    7     int *auth_code_ptr; // 指向验证码的指针
    8 };
    9
 ► 10 int calculate_key(int n) {
   11     if (n <= 1) return 5;
   12     return n * 2 + calculate_key(n - 1);
   13 }
   14
   15 void login() {
   16     int secret_code = 2026;
   17     struct User jason;
───────────────────────────────────[ STACK ]────────────────────────────────────
00:0000│ rsp   0x7fffffffdbd8 —▸ 0x5555555551d3 (login+75) ◂— mov edx, eax
01:0008│-0e0   0x7fffffffdbe0 ◂— 0
02:0010│ rax-4 0x7fffffffdbe8 ◂— 0x7ea00000000
03:0018│-0d0   0x7fffffffdbf0 ◂— 0x6e696d6461 /* 'admin' */
04:0020│-0c8   0x7fffffffdbf8 ◂— 1
05:0028│-0c0   0x7fffffffdc00 —▸ 0x7fffffffdbec ◂— 0x696d6461000007ea
06:0030│-0b8   0x7fffffffdc08 ◂— 0xd1d141296e12fc00
07:0038│-0b0   0x7fffffffdc10 —▸ 0x7fffffffdd00 —▸ 0x555555555080 (_start) ◂— endbr64
─────────────────────────────────[ BACKTRACE ]──────────────────────────────────
 ► 0   0x555555555169 calculate_key
   1   0x5555555551d3 login+75
   2   0x55555555521c main+18
   3   0x7ffff7c2a1ca __libc_start_call_main+122
   4   0x7ffff7c2a28b __libc_start_main+139
   5   0x5555555550a5 _start+37
```

观察：

```bash
 ► 0x555555555169 <calculate_key>       endbr64
   0x55555555516d <calculate_key+4>     cmp    edi, 1     3 - 1     EFLAGS => 0x202 [ cf pf af zf sf IF df of iopl:00 ac ]
   0x555555555170 <calculate_key+7>   ✘ jle    calculate_key+25            <calculate_key+25>

   0x555555555172 <calculate_key+9>     push   rbx
```

首先rsp的值-8，存的是之前下一条指令的地址；实际上3>1所以不发生跳转，顺序执行下一条指令。

```bash
 ► 0x555555555172 <calculate_key+9>     push   rbx
   0x555555555173 <calculate_key+10>    lea    ebx, [rdi + rdi]     EBX => 6
   0x555555555176 <calculate_key+13>    sub    edi, 1               EDI => 2 (3 - 1)
   0x555555555179 <calculate_key+16>    call   calculate_key               <calculate_key>
```

首先rbx是基址寄存器，也是被调用者保存，后续想用rbx，所以要先保护里面的内容，压栈。

然后编译器优化了，不做乘法优化为加法即`rdi + rdi`这样更快。

### 递归退出

```bash
   0x555555555169 <calculate_key>       endbr64
 ► 0x55555555516d <calculate_key+4>     cmp    edi, 1     1 - 1     EFLAGS => 0x246 [ cf PF af ZF sf IF df of iopl:00 ac ]
   0x555555555170 <calculate_key+7>   ✔ jle    calculate_key+25            <calculate_key+25>
    ↓
   0x555555555182 <calculate_key+25>    mov    eax, 5     EAX => 5
   0x555555555187 <calculate_key+30>    ret                                <calculate_key+21>
    ↓
   0x55555555517e <calculate_key+21>    add    eax, ebx     EAX => 9 (5 + 4)
   0x555555555180 <calculate_key+23>    pop    rbx          RBX => 6
   0x555555555181 <calculate_key+24>    ret                                <calculate_key+21>
    ↓
   0x55555555517e <calculate_key+21>    add    eax, ebx     EAX => 0xf (9 + 6)
   0x555555555180 <calculate_key+23>    pop    rbx          RBX => 0x7fffffffdd48
   0x555555555181 <calculate_key+24>    ret                                <login+75>
───────────────────────────────[ SOURCE (CODE) ]────────────────────────────────
In file: /home/jason/gdb/src/challenge.c:11
    5     char username[8];
    6     int level;         // 用户等级
    7     int *auth_code_ptr; // 指向验证码的指针
    8 };
    9
   10 int calculate_key(int n) {
 ► 11     if (n <= 1) return 5;
   12     return n * 2 + calculate_key(n - 1);
   13 }
   14
   15 void login() {
   16     int secret_code = 2026;
   17     struct User jason;
   18
───────────────────────────────────[ STACK ]────────────────────────────────────
00:0000│ rsp   0x7fffffffdbb8 —▸ 0x55555555517e (calculate_key+21) ◂— add eax, ebx
01:0008│-100   0x7fffffffdbc0 ◂— 6
02:0010│-0f8   0x7fffffffdbc8 —▸ 0x55555555517e (calculate_key+21) ◂— add eax, ebx
03:0018│-0f0   0x7fffffffdbd0 —▸ 0x7fffffffdd48 —▸ 0x7fffffffe041 ◂— '/home/jason/gdb/src/challenge'
04:0020│-0e8   0x7fffffffdbd8 —▸ 0x5555555551d3 (login+75) ◂— mov edx, eax
05:0028│-0e0   0x7fffffffdbe0 ◂— 0
06:0030│ rax-4 0x7fffffffdbe8 ◂— 0x7ea00000000
07:0038│-0d0   0x7fffffffdbf0 ◂— 0x6e696d6461 /* 'admin' */
─────────────────────────────────[ BACKTRACE ]──────────────────────────────────
 ► 0   0x55555555516d calculate_key+4
   1   0x55555555517e calculate_key+21
   2   0x55555555517e calculate_key+21
   3   0x5555555551d3 login+75
   4   0x55555555521c main+18
   5   0x7ffff7c2a1ca __libc_start_call_main+122
   6   0x7ffff7c2a28b __libc_start_main+139
   7   0x5555555550a5 _start+37
```

当前EAX内容为15.但为了“通过”，设置为17，level也修改为大于100：

```bash
set $eax = 17

RAX  0x11 # 16+1=17
```

### __printf_chk

这可以说是printf加强版，它第一个参数 EDI = 2 其实是一个安全级别标志。它会检查你的格式化字符串（RSI）里有没有危险的操作，防止恶意攻击。调用这个时，传递参数的顺序：

1. rdi 安全级别（当前是2）
2. rsi 字符串地址
3. rdx 第一个%d
4. rcx 第二个%d

## TASK_C

修改Level

```bash
set {int}($rbp - 0x18) = 1337
```

修改后的反编译情况为：

```bash
──────────────────────[ DISASM / x86-64 / set emulate on ]──────────────────────
   0x5555555551fa <login+57>    lea    rax, [rbp - 0x28]
   0x5555555551fe <login+61>    mov    qword ptr [rbp - 0x10], rax
   0x555555555202 <login+65>    mov    edi, 3                          EDI => 3
   0x555555555207 <login+70>    call   calculate_key               <calculate_key>

   0x55555555520c <login+75>    mov    dword ptr [rbp - 0x24], eax
 ► 0x55555555520f <login+78>    cmp    dword ptr [rbp - 0x24], 0x11     0x11 - 0x11     EFLAGS => 0x246 [ cf PF af ZF sf IF df of iopl:00 ac ]
   0x555555555213 <login+82>  ✘ jne    login+109                   <login+109>

   0x555555555215 <login+84>    mov    eax, dword ptr [rbp - 0x18]      EAX, [{jason+0x8}] => 0x539
   0x555555555218 <login+87>    cmp    eax, 0x64                        0x539 - 0x64     EFLAGS => 0x202 [ cf pf af zf sf IF df of iopl:00 ac ]
   0x55555555521b <login+90>  ✘ jle    login+109                   <login+109>

   0x55555555521d <login+92>    lea    rax, [rip + 0xde4]               RAX => 0x555555556008 ◂— '--- ACCESS GRANTED: WELCOME MASTER ---'
───────────────────────────────[ SOURCE (CODE) ]────────────────────────────────
```

key和level校验通过，不再跳转了。

## Canary验证

```bash
   0x55555555522c <login+107>    jmp    login+137                   <login+137>
    ↓
   0x55555555524a <login+137>    nop
 ► 0x55555555524b <login+138>    mov    rax, qword ptr [rbp - 8]     RAX, [0x7fffffffdc08] => 0xd9cafab2ae169d00
   0x55555555524f <login+142>    sub    rax, qword ptr fs:[0x28]     RAX => 0 (0xd9cafab2ae169d00 - 0xd9cafab2ae169d00)
   0x555555555258 <login+151>  ✔ je     login+158                   <login+158>
```

