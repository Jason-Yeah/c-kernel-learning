# CS:APP · Motion

HTML + SVG + GSAP 制作的分镜式计算机原理动画：纯黑背景、普通文字纯白、颜色只用于区分对象和状态，寄存器与关键标签放大加粗。当前 18 部动画、116 个镜头，采用共享播放器，支持自动播放、暂停、单步、前后关键步骤、进度拖动、键盘和全屏。当前版本无配音。**七个既有主题、独立 OS 导读与按计划补充的第1章取指／译码／执行、第2章字节序、C位运算、补码／转换／溢出都有完整 C 与真实调试证据。字节序采用跨镜头共享对象；先前适合的13片也已同步回改，35组主体／43处切镜按需保留，不默认整屏清空。递归原匹配转场与30秒机制保留。OS 新章另提供实际 ELF／readelf／objdump 输出；可观测事实与硬件／内核机制模型明确分开。**

## 本地观看

```bash
cd /home/jason/study/ck_study/animation
npm ci
npm start
```

浏览器打开 **http://localhost:4173/**，按“章节 → 主题 → 动画”进入。动画结束保留末帧；加 `?paused` 可暂停打开。模块页面请通过 HTTP 服务观看，不直接双击 HTML。

| 章节 | 动画 | 时长 |
| --- | --- | --- |
| [独立 OS 基础章](chapter0/index.html) | [C → 编译／汇编／链接](chapter0/compile.html) | 64s |
| OS 基础 | [可重定位 ELF 结构](chapter0/elf-object.html) | 64s |
| OS 基础 | [exec 与装载](chapter0/loading.html) | 80s |
| OS 基础 | [进程、虚拟化与内存分布](chapter0/process-space.html) | 80s |
| OS 基础 | [父子进程的地址与私有／共享](chapter0/parent-child.html) | 64s |
| OS 基础 | [x86-64 寄存器](chapter0/registers.html) | 80s |
| [第1章](chapter1/index.html) | [取指、译码、执行与写回](chapter1/instruction-cycle.html) | 96s |
| [第2章](chapter2/index.html) | [字节序与读取解释](chapter2/byte-order.html) | 96s |
| 第2章 | [C位运算：类型、掩码与移位](chapter2/bitwise.html) | 96s |
| 第2章 | [补码、扩展、截断与溢出](chapter2/integers.html) | 128s |
| [第 3 章](chapter3/index.html) | [函数调用与栈帧 · GDB 实测](chapter3/stack-frame.html) | 48s |
| 第 3 章 | [栈缓冲区越界写](chapter3/stack-overflow.html) | 74s |
| 第 3 章 | [调用栈空间耗尽](chapter3/stack-exhaustion-film.html) | 42s（含原 30s 机制段） |
| [第 6 章](chapter6/index.html) | [Cache](chapter6/cache.html) | 84s |
| [第 8 章](chapter8/index.html) | [异常、中断、系统调用与上下文切换](chapter8/exception-control.html) | 124s |
| 第 8 章 | [进程生命周期](chapter8/process-lifecycle.html) | 78s |
| 第 8 章 | [父子进程内存与内核布局](chapter8/fork-memory-layout.html) | 76s |
| [第 9 章](chapter9/index.html) | [虚拟地址翻译](chapter9/address-translation.html) | 90s |

## 从真实实验采集到动画

```bash
npm run capture:all     # 原实验 + OS + 指令周期 + 字节序 + 整数 + 异常控制流
npm run capture:os      # 仅 OS 基础章，3次运行 / 51个停点
npm run capture:cycle   # 第1章取指译码执行，逐指令实际采集
npm run capture:byte-order # 第2章字节序，12个实际观察停点
npm run capture:integers   # C位运算/整数，17个GDB停点 + 同源UBSan
npm run capture:control    # 第8章：2次GDB /10停点 + 1次独立OS阻塞read
npm test                # 原始记录、完整源码、动画、工作台与完整播放
```

[七份完整 C 实验](experiments/README.md)、[采样说明](traces/README.md)与[导演方案](docs/experiment-programs.md)包含实际参数和观测边界。原七实验为15次运行、193个停点；新 OS 实验为3次运行、51个停点及9份工具报告，第1章指令实验另增加1次运行、18个停点，字节序另增1次运行、12个停点，整数／位运算再增1次GDB运行、17个停点，再加第8章2次GDB／10停点，合计保存23次GDB运行、301个停点；第8章另有1次非GDB阻塞read的OS观察，整数同源UBSan校验报告保留。源码／ELF SHA-256、工具版本、编译命令与 ASLR 随数据保存。

六片新增“C 程序与 GDB”镜头，页面下方实验台可查看完整程序、选择独立运行和实际停点，核对原样命令／输出，并定位对应讲解；原栈帧仍全程真实数据驱动。缓冲区、PID、六类 fork 对象读值和实际信号不再靠手写数值。

网页是离线回放，不是在线运行 GDB。Cache/TLB、PFN、COW 物理复制与调度等不可直接观测部分仍标为机制模型；不会因为有程序就冒充硬件实测。采集器只支持这些已审查的固定实验；用户代码需先核对具体文件、构建与观测点。

## 独立 OS 基础章

从 [chapter0/index.html](chapter0/index.html) 进入六节动画，或直接阅读 [完整 HTML 讲义](chapter0/guide.html)（单一原稿为 [os-basics.md](chapter0/os-basics.md)）。内容覆盖用户要求的进程抽象／地址分布、父子地址空间、编译链接装载、可重定位 .o 和 x86-64 寄存器。

新章内存一律用竖向分块数组，高 VA 在上、低 VA 在下；父子各一列。磁盘文件另标 offset 向下增加，物理页与调度示意不冒充实测。每片有完整三文件 C、ELF、工具报告和 GDB 选择／定位实验台。详见 [分镜](docs/os-basics-storyboards.md)。

## 第2章：C位运算与整数

新增[C位运算](chapter2/bitwise.html)与[补码／扩展／截断／溢出](chapter2/integers.html)两片，配套[网页讲义](chapter2/guide.html)及[单一原稿](chapter2/integers.md)。一份安全C、17个GDB停点、40个带实际类型／宽度／地址／字节的对象、10条表达式类型证据与同源UBSan报告可核对。

保留源值、位串、类型和仍需使用的结果。明确uint8_t的整型提升、32位NOT／移位、补码负权、补0／符号扩展、unsigned转换与模加；signed溢出和非法移位被拦截，没有伪造回绕结果。逐位运动是数学解释，不冒充CPU逐位执行。浮点仍未开始。

## 第8章：直接进入异常控制流

按最新要求，第3章后续补充不做，第二章剩余内容继续暂停；已有动画保留。新增[8-1异常／中断／系统调用／上下文切换](chapter8/exception-control.html)，124s／8镜头，配套[讲义](chapter8/guide.html)、[分镜](docs/control-flow-storyboards.md)和完整control-flow.c。

实际用户CALL／SYSCALL边界、保护页SIGSEGV，另用非GDB运行观察空管道read的S状态、受控唤醒、1B复制、rusage变化与helper回收。只限制本目标到一个原allowed CPU，不改宿主；内核／IRQ／调度箭头明确为模型，不把进入内核画成必定换进程。信号完整专题尚未开始。

## 静态发布 / GitHub Pages

```bash
npm run build
```

发布 **`dist/` 的内容**即可，包括 `index.html`、各章目录、动画页面和打包资源。GSAP 已打包，无需发布 `node_modules`、测试、录屏或 Node 预览服务。相对链接已实测兼容项目子目录，例如 `yourname.github.io/csapp/`。本轮没有推送或部署任何远程仓库。

## 验证

```bash
npx playwright install chromium   # 测试环境首次准备浏览器
npm test                         # 构建、模型、C 示例、十八片真实浏览器回归
```

- `npm run test:models`：教学模型与实测采样不变量、源码哈希、原始字节断言。
- `npm run test:measured`：独立读取原始 GDB JSON，逐项核对源码／发布版的寄存器、地址、源码行号与证据链接。
- `npm run test:buffer`：独立核对全部 44 次真实复制的 input 保留、buf 八格容量、实际写前值／框外栈字节、路径与正反向定位。
- `npm run test:experiments`：六主题的完整 C、真实 GDB 命令／停点、实验镜头、实际值与工作台定位，源码和发布版均核对。
- `npm run test:cycle`：独立核对真实指令字节、有效地址、寄存器／内存前后、源保留和工作台。
- `npm run test:byte-order`：独立核对真实字节、BE提交／读回、跨切镜同一DOM／数据保留／无闪空、运动遮挡及实验台。
- `npm run test:integers`：两片的原始C/GDB、实际类型／位宽／编码／范围、源和结果同节点保留、真实跨切镜、反向定位、运动遮挡与讲义。
- `npm run test:control`：独立核对CALL／SYSCALL／故障、原始/proc／rusage／read、运行身份、跨切镜原节点、运动遮挡、两个实验台与讲义。
- `npm run test:continuity`：13部旧片的35组原对象、43处实际切镜、原元素不替换／值和地址连续、Cache逐次提交、运动／面板遮挡、错误身份拒绝及递归原匹配构图。
- `npm run test:os`：独立原始 ELF／GDB、地址方向、父子读值、ABI／标志、完整讲义与工作台核对。
- `npm run test:example`：Linux C 示例在 -O0/-O2 下的 8 次运行。
- `npm run test:browser`：需要已构建 dist；116 个镜头、控件、实际完整播放及发布子路径；检查纯黑无渐变背景、普通白字、寄存器字号与长地址不截断。
- 截图、接触表、录屏与验证报告生成在 `film-artifacts/`，不发布到站点。

## 维护

- [AGENTS.md](AGENTS.md)：后续动画的强制规范。
- [导演分镜](docs/storyboards.md)：先设计再实现。
- [范围与进度](docs/spec.md)、[验证记录](docs/verification.md)、[旧版清理记录](docs/migration.md)。
- [后续章节优先级评估](docs/chapter-priorities.md)：历史评估；最新已按明确要求直接完成第8章8-1，第3章后续跳过，第二章剩余暂缓。
- `shared/`：唯一播放器、视觉样式、SVG组件、轨迹与进退场工具；新片用continuity持久层，旧构图可用actorContinuity挂接原对象，详见[回改分镜与约束](docs/continuity-retrofit-storyboards.md)。
- `models/`：可核算、可测试的教学数据。
- `chapterN/`：主题模块与页面。HTML 目录和入口由 `scripts/pages.mjs` 根据 `shared/catalog.js` 生成；修改目录数据后运行 `npm run pages` 或构建。
- [父子进程 C 示例](chapter8/fork-memory-demo.c)与[说明](chapter8/fork-memory-layout.md)继续保留。

旧 SVG、Python 生成器和废弃播放器已在新版通过验证后清理，不再维护两套实现。尚未实测 Firefox、Safari、真实手机及屏幕阅读器；移动视口和触摸仅用 Chromium 仿真检查。
