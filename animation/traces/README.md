# 实际调试采样

七个对应 C 程序：stack-call、buffer-copy、stack-recursion、cache-access、process-lifecycle、fork-memory、vm-access。每个都有同名 JSON 与 `*-gdb.txt`，由 `npm run capture:all` 实际编译、运行和验证后生成。

- stack-call.json：main/caller/add 的 31 个逐指令停点与 12 个命名事件，保持原有 schema。
- 其他六份 JSON：provenance 记录完整源码、源码／ELF SHA-256、工具、参数、ASLR、时间与观测范围；runs 按独立运行参数分组；samples 保存真实 PID、指令／源行／调用位置、寄存器、字段与原样执行的 GDB commands/output。
- `outcome=signal` 是实际信号停点；目标在记录后由 GDB 清理，不把清理当作正常退出。多进程的 exits 单独记录各 inferior 的真实退出码。
- 地址与 64 位寄存器使用十六进制字符串，地址运算用 BigInt；al 按八位比特解释，保留原始 GDB 的有符号显示供核对。
- VM 的 page 只存 pagemap 标志；不把受权限屏蔽的 PFN 或零字段当成真实物理页号。冷页在程序访问前不被调试器提前解引用。

原七实验为15次运行、193个停点。新增 os-basics.json / os-basics-gdb.txt 是 OS 基础章的三文件程序：另有3次运行、51个停点及9份 binutils 输出，随后instruction-cycle.json / instruction-cycle-gdb.txt新增1次运行、18个逐指令停点，此后byte-order.json / byte-order-gdb.txt增加1次运行、12个停点，随后integer-bits.json／integer-bits-gdb.txt再新增保存1次GDB运行、17停点，其后control-flow.json新增保存2次GDB运行／10停点，当前合计23次GDB运行／301停点，另有control-flow的1次非GDB OS观察（既有同源UBSan校验另计）。字节序记录同一word的原字节与不变地址、BE数组四次CC到编码值的提交及三种实际读值；受控ELF在traces/byte-order中显式发布，不由网页执行。指令周期片保留所有指令字节、寄存器、输入／输出内存与实际命令，traces/instruction-cycle中发布受控ELF和反汇编；内部取指／译码过程为机制解释。OS的JSON另存完整ELF头／节／段／符号／重定位、实际PC32补丁和多源码哈希；`traces/os-basics/` 保存 .i/.s/.o 与受控 .elf 文件证据，它们不会被网站执行。再次运行时地址、PID、递归层数等可变化；场景应由采样更新，不在 renderer 中写入“新预期地址”。既有实验 ELF 与临时日志仍留在忽略版本管理的 .trace-build；OS 导读为查看文件结构，另显式发布其受控 .o/.elf 文件证据，访客不需要执行或安装调试器。

integer-bits记录C位运算与整数的同一次执行：17停点／40个对象的实际类型、宽度、有符号性、数值、地址、字节及原样命令，八个寄存器，另有10条实际表达式类型。unsigned打印用p/u、signed用p/d。traces/integer-bits/program.elf与sanitizer.txt是显式下载证据；UBSan正常退出0且无诊断，非法移位和int32 signed溢出被检查拦截，不能把未赋值的0当作它们的结果。两片共用同一程序不同观察，位权／按位推导／进位不是CPU时钟测量。

control-flow.json包含inspect／fault两个GDB运行：普通CALL／实际SYSCALL边界和自己的保护页故障，10停点及原始用户寄存器／字节／信号／源码。另有os.blocking的独立程序stdout、/proc原文件读取、CPU允许列表／阻塞read参数／rusage／回收。traces/control-flow/发布受控ELF和blocking-os.txt；gdb文本也保存原batch停止／退出通知。内核入口、调度箭头和硬件IRQ不是这些用户态采样；故障后的0不当作成功读回值，不由GDB自动打印字符指针读取保护页。

页面实验台显示完整源码与命令，可选择停点并定位对应讲解。原始文本和 JSON 均可直接查看。解释性相机、速度和不可观测的硬件／内核机制不属于这些采样。详见 [实验说明](../experiments/README.md)。
