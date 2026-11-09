# 配套 C、GDB 与 ELF 实验

这不是动画旁的几行示意代码。experiments 中每个程序均已实际编译、由 GDB 采集；页面提供完整程序、实际命令／输出、采集 JSON、源码／ELF SHA-256 和可定位的停点。

原七片迁移时新增六个独立实验，原有 stack-call 继续保留；随后增加OS导读、指令周期、字节序、C位运算／整数和第8章异常控制流，见下方专节。用户学习代码 `chapter8/fork-memory-demo.c` 不改逻辑，仍保留并独立测试；它与本轮采集用的 fork-memory.c 分开标注，不能混淆来源。

## 本机重现

环境：Linux x86-64（本机为 WSL2）、GCC、含 Python 的 GDB、Node.js。实际采用的工具版本和完整参数以各 JSON 的 provenance 为准。

```bash
cd /home/jason/study/ck_study/animation
npm ci
npm run capture:all          # 保存23次GDB／301停点；另有本次OS观察与既有UBSan
npm test                    # 数据、源码哈希、浏览器和完整播放
npm start                   # http://localhost:4173/
```

只重采一个补充主题：

```bash
npm run capture:experiments -- buffer-copy
npm run capture:experiments -- vm-access
npm run capture:stack        # 原 main/caller/add 逐指令采集
```

采集成功后重新 `npm run build` 才更新发布版。网站读取离线数据，**不会在访客浏览器中执行 C 或实时控制 GDB**。`npm test` 默认验证已保存数据，不要求重新运行 GDB。

## 第8章异常控制流：GDB边界与独立OS观察

[control-flow.c](control-flow.c)对应[8-1](../chapter8/exception-control.html)与[讲义](../chapter8/guide.html)。`npm run capture:control`实际编译后采集inspect的7个GDB停点、fault的3个停点，再独立运行blocking并读取/proc；二者不是同一次进程执行。

inspect记录local_add真实参数／返回和SYSCALL 0F05前后用户状态；raw_getpid为自写GNU asm包装，C的CALL仍在用户态。fault只读自己的PROT_NONE映射，记录同RIP／SIGSEGV／si_code／si_addr；不执行C层任意非法指针解引用。关闭GDB frame-arguments打印以免它自动把字符指针当字符串读出，保护后不提前读取目标，信号终止不伪报正常退出。

blocking在非GDB下只将本目标及helper限制到一个原allowed CPU；父空管道read被/proc实际观察到S、read号／fd／缓冲区／长度1，helper等collector控制!再写业务K。接收字节CC→4B、返回1、rusage变化、wait回收都有原始stdout；!不是业务数据。计数和状态不证明完整调度轨迹，kernel／IRQ图仍是模型，不修改宿主配置或假造实测内核PC。受控ELF及blocking-os.txt为下载证据，不由网页执行。

第3章后续按要求跳过，本例普通函数仅作第8章对照；不重做调用栈。信号递送／屏蔽留作后续。

## C位运算与整数：同一完整程序的两个主题

[integer-bits.c](integer-bits.c)对应[位运算](../chapter2/bitwise.html)、[补码／扩展／截断／溢出](../chapter2/integers.html)和[讲义](../chapter2/guide.html)。`npm run capture:integers`实际编译、采集17个GDB停点／40个带类型和字节的对象／10条表达式类型，再实际运行同源UBSan构建，正常退出0、stderr为空；报告与受控ELF在traces/integer-bits/。

本例C11／Linux x86-64，8-bit byte、32-bit int。unsigned ~bits为FFFFFF49，small的~先提升为int得-183，再转uint8得49；左移2得到2D8，不丢高位。u8／s8的实际编码均D6而数值为214／-42，扩展得到00D6／FFD6，unsigned窄化1234→34，uint32模加得到0。INT32_MAX+1先升到int64再检查拒绝，没有执行signed溢出；count32也被拦截。int8_t的127+1先在int中得128，不能误教成8位signed加法。

所有全局值已显式或静态初始化；guarded_shift／signed_result保留的0不是非法运算的结果。GDB按实际有符号性选择p/d或p/u，不将强制有符号格式误读为unsigned对象的C值。两部动画复用的是同一次已保存GDB执行的不同观察，不伪装成不同机器；数学逐位推导不等于CPU微架构时钟。新例不关闭宿主保护或依赖UB，GNU观察标记仅用于受控调试。

## 程序、参数与真实结果（原七片）

| 程序 | 参数 | 已实际观察 |
| --- | --- | --- |
| [stack-call.c](stack-call.c) | 无 | main→caller(3)→add(3,2)，真实寄存器、返回地址与结果 5；31 个指令停点 |
| [buffer-copy.c](buffer-copy.c) | safe / edge / broken | 8／9／27 次逐字节写入；真实 input、buf 与栈槽；前两次返回，第三次实际停在 SIGSEGV |
| [stack-recursion.c](stack-recursion.c) | exhaust / bounded | 本实验栈上限 256KiB；实际每层 rsp、栈访问失败及回溯；独立 bounded 运行四层返回 sum=10 |
| [cache-access.c](cache-access.c) | 无 | 对齐数组的实际 VA、完整 256B 内容、八次读取的偏移／数据／AL |
| [process-lifecycle.c](process-lifecycle.c) | early / late | 真实 fork 返回和 PID、catch exec、同 PID 的 worker 模式、PC／CP、真实 Z、waitpid 和 0x0700 |
| [fork-memory.c](fork-memory.c) | 无 | .data／.bss／local／malloc／MAP_PRIVATE 的父子隔离，MAP_SHARED 写入可见，实际 VA 和 maps 权限 |
| [vm-access.c](vm-access.c) | resident-a / resident-b / cold / readonly / unmapped | 实机 4KiB 页、实际 VA、驻留位变化、读值，以及 ACCERR／MAPERR 的真实 si_addr |

原七片采集基线为 **15 次运行、193 个停点**：stack-call 31 个逐指令停点，补充六主题 162 个断点／信号停点。重采时地址、PID、递归最大层数等可以变化，动画不依赖旧地址常量。

## 怎么看程序与调试

- 六个新增接入的主题先有独立“C 程序与 GDB”镜头，再进入原机制动画；栈帧主题原本已是全程 C／GDB 驱动。
- 页面下方“完整 C 程序与实际 GDB 调试记录”默认展开。选择独立运行和停点，可以查看完整源码、实际命令、寄存器、原始结果与信号。
- “定位这个实验停点”回到程序／GDB 镜头；“定位对应机制段”表示与观测相关的解释性演示，不伪装成逐指令 CPU 录屏。缓冲区可以定位单次字节写入，故障可跳到对应异常段。
- 主画面只显示相关源句，省略采集用的 trace_phase、nop、checkpoint 和注释，**保留实际源码行号**；完整程序和原始记录不作删改。
- 完整原始调试文本在 traces 下，各主题均可从工作台直接打开；“数据来源”面板记录工具、参数、ASLR、时间和哈希。

也可以手动调试已经编译的本地 ELF，例如：

```bash
gdb --args .trace-build/buffer-copy safe
gdb --args .trace-build/stack-recursion exhaust
gdb --args .trace-build/process-lifecycle late
```

在 GDB 中先 `set disable-randomization off`。源码中的 `TRACE_*` 注释标出自动采集点，工作台显示本次对应行号。对多进程程序，自动采集器采用 `set detach-on-fork off`、`set follow-fork-mode parent`、`set schedule-multiple on`，生命周期实验还使用 `catch exec`。不要仅跟随一方就把管道阻塞误认为程序出错。

## 编译与观测条件

- 完整命令以 JSON 的 compilerCommand 为准。原七片使用 -O0、-g3、PIE、帧指针、禁用尾调用／过程间栈对齐优化和 red zone，便于观察当前固定实验。
- 原六个补充实验显式使用 -fno-stack-protector、关闭本 ELF 的 fortify；这只是该小实验的构建条件，不修改宿主系统保护，不能推广为默认编译行为。
- ASLR 保留，不改系统 sysctl。所有地址为**进程虚拟地址**，不是物理页号。
- 故障实验禁用 core dump，递归仅降低本进程的栈资源上限。GDB 在预期信号处记录后清理目标；JSON 中 outcome=signal，不把调试器清理误报为正常退出。
- GDB 可能把 rax／al 按有符号整数显示；JSON 按 64／8 位比特保存，测试与原始寄存器输出核对，不把负号当地址。
- 函数入口处的 DWARF 变量可能尚未初始化。stack-call 入口先读参数寄存器，其他数据仅取明确初始化后的停点；失败帧不冒读无效 depth。

## 每个主题的边界

### 缓冲区

数组明确初始化为 CC，画面原值来自实际快照，不再用手写未知值。`_Alignas(16)` 是受控选择，本次观测 buf 到 rbp／返回槽的距离为 16／24B；换编译条件不保证一致。逐字节循环模拟字符串复制，不声称 glibc strcpy 的真实实现按字节运行。

越界为 C 未定义行为。只解释这次已记录的编译与运行，不保证所有机器相同。尤其 AD DE 00 只改返回地址低字节，**真实目标保留原高位，不是整条地址必然等于 0xDEAD**。

### 递归

实际最大层数依赖本次地址空间／初始栈和 256KiB 上限。画面只选八个代表帧，不代表只能递归八次；失败镜头使用实际层数、信号和地址。正常返回是另一进程的 bounded 运行，不能理解成溢出后自动恢复。

### Cache、VM 与 fork

- GDB 证实 C 的地址、加载、字节、权限、进程数据与信号；不能直接读取真实 Cache tag／LRU／命中记录或全部 TLB 条目。
- Cache 模型用实际 C 的请求序列和数组字节驱动，但 4组×2路×8B、HIT／MISS 是明确模型，不是 PMU 实测。
- VM 的实机页为 4KiB，旧机制模型为 64B；二者分开标注。首次匿名读取可能映射 Linux 共享零页，present 从 0 到 1 不等于证明分配了一个新私有物理页。
- cold 访问前，采集器只读指针、maps 和 pagemap 标志，不用 x 命令或 read_memory 提前读目标页；否则会由调试器自己把冷页触入内存。minor-fault 增量仍可能包括调试开销，不冒充精确硬件延迟。
- 同 VA／maps 与私有值隔离不是 PFN 共享、COW 复制或 PTE 更新的直接观测。对应物理页、页表、TLB、内核结构和调度段使用明确的机制模型身份。

## 独立 OS 基础章的三文件实验

[os-main.c](os-main.c)、[os-math.c](os-math.c)、[os-demo.h](os-demo.h) 组成一个安全的双翻译单元程序。`npm run capture:os` 实际执行 GCC -E/-S/-c/链接、readelf/objdump/nm 和 GDB，保存3次运行、51个停点、9份 binutils 报告；源码、.i/.s/.o/ELF 都有 SHA-256。

两个 inspect 运行分别观察 PT_INTERP 的入口、应用 _start、main、对象初始化、真实参数／返回、EAX 清高位和 cmp 标志；fork 运行用管道观察六类对象的父前／子前／子后／父后。实际 R_X86_64_PC32 的四字节补丁与 S+A-P、最终文件字节和 objdump 独立核对。

[基础章讲义](../chapter0/guide.html)和[原稿](../chapter0/os-basics.md)提供完整解释。`traces/os-basics/` 中的 .o 和受控 os-demo.elf 是可下载的文件证据，网站不会执行。物理框、COW 内部、装载内核操作和 CPU 调度仍为明确机制解释；不从 VA 推断 PFN。

## 按计划补第1章：取指／译码／执行

[instruction-cycle.c](instruction-cycle.c) 是完整安全C程序：两个全局输入3／2，calculate相加，写observed并返回5。`npm run capture:cycle` 使用真实GCC／GDB单步，当前18个停点、9个命名事件。原始字节与发布ELF核对；MOV的RIP-relative有效地址、32位ADD前后、唯一store提交、RET返回地址都验证。原样命令、源码／ELF哈希与ASLR保留。

[片目与实验说明](../chapter1/instruction-cycle.md)。网页保留完整C、全部中间停点、指令前后对照和相关讲解定位；内部取指／译码／ALU为明确模型，非真实硬件时钟、流水线或总线。输入始终保留，输出在store后统一提交，不造逐字节总线时序。

## 第2章：字节序与连续分镜

[byte-order.c](byte-order.c) 保留同一word=0x12345678，合法通过unsigned char读取四个原字节；be_encoded[4]初始化CC后显式生成BE编码。memcpy原word得12345678，按本机序直接读BE数组得78563412，按BE规则解码恢复12345678。它不是在大端CPU上采集，也不使用未对齐uint32_t*解引用。

npm run capture:byte-order 实际编译、运行、由GDB保存12个停点及源码／ELF哈希，正常退出0。主片96秒／6镜头，word、地址和内存按需要跨镜头持续保留；实验台展示所有实际命令、源行与读写值，含单次BE写入定位。[本片说明](../chapter2/byte-order.md)。累计保存20次运行／274停点。

采集器只支持这些已审查的固定程序与参数，不是任意 C/C++ 程序自动转动画的工具。接入用户现有程序时，先检查具体源文件、构建参数、同步和观测点，再适配采集；缺事件、源码哈希或断言失败必须报错，不补造数据。
