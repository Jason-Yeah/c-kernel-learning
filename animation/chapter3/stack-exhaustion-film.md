# 调用栈为什么会溢出？——C/GDB 实验与原 30 秒机制段

主题仍为**递归过深造成调用栈空间耗尽**，不同于[栈缓冲区越界写](stack-overflow.html)。保留原来用户确认的五镜头机制和约 30 秒节奏，本轮前置 12 秒完整程序／GDB 实验，入口总时长为 42 秒。

## 观看与实验

在项目目录 `npm ci && npm start`，打开 <http://localhost:4173/chapter3/stack-exhaustion-film.html>，或从[第 3 章目录](index.html)进入。

对应完整程序：[stack-recursion.c](../experiments/stack-recursion.c)，真实 [GDB 文本](../traces/stack-recursion-gdb.txt)与 [JSON](../traces/stack-recursion.json)可核对。页面实验台可以选择 exhaust／bounded 两次独立运行和各个停点，查看完整程序、寄存器／回溯与命令输出。

| 镜头 | 当前入口实际时间 | 主要动作 |
| --- | --- | --- |
| C 程序与 GDB | 0–12s | 四个实际深度／rsp／4096B 局部数组停点，解释数据来自哪段程序 |
| 一次调用 | 12–16.5s | 从 f(1) 开始建立首个代表帧 |
| 再次调用 | 16.5–22.5s | 上层未返回，继续到 f(4) |
| 空间有限 | 22.5–28.5s | 拉远到实测层数，选八帧，其余折叠；计数与实际代表帧编号同步 |
| 触及上限 | 28.5–34.5s | 新层栈访问产生实际 SIGSEGV，不覆盖旧帧；显示实测信号地址 |
| 正常返回对照 | 34.5–42s | 明确切换独立 bounded 运行，四层逐层返回，sum=10 |

连续衔接回改时已核对原有三处匹配构图：call→递归、递归→容量、容量→最后有效帧，原本就没有全屏清空或几何跳变，因此保持这段已批准的镜头，不为实现统一改掉节奏；故障→正常路径仍是独立运行对照。

原五镜头仍按 20 秒制作坐标、stretch=1.5 得到 30 秒。前置实验由同一教学时钟处理，未把所有段加速。分镜见 [storyboards.md](../docs/storyboards.md) 和 [实验方案](../docs/experiment-programs.md)。

## 观测与模型边界

- 本次 C 实验主动设置**本进程** 256KiB 栈上限，每层 volatile block[4096]，保留帧指针、禁用尾调用等；不是宿主默认栈大小或所有 C 程序的栈帧。
- 实际 rsp、每层 stride、最大完整层数、失败地址和回溯来自采样。画面只选八个代表帧，不能说“最多递归八次”。
- 失败帧的 C 参数可能未写入，采集器不从无效栈位置冒读 depth。虚线表示未完成的新帧，不是已成功分配或覆盖旧内容。
- 有终止条件的四层返回是另一次进程运行，不暗示溢出后恢复；有终止条件也可能递归过深。
- 压缩几何、箭头和教学速度仍是机制解释。释放帧不等于清零内存。

## 当前文件与验收

stack-exhaustion-film.js 配置统一播放器；五个 scene 模块保留独立 Enter／Action／Hold／Exit；shared/experimental-film.js 生成独立程序／GDB 镜头，shared/experiment-view.js 提供完整实验台。旧重复播放器已清理。

`npm run capture:experiments -- stack-recursion` 重采；`npm test` 构建并核对。已在 Chromium 原速完整运行 42 秒，回归显隐、任意回退、控件、触摸仿真、真实数据与完整程序。详见 [验收记录](../docs/verification.md)。

dist 仍可静态托管，没有实际部署远端。当前无配音，Firefox、Safari、真实手机和屏幕阅读器未实测。
