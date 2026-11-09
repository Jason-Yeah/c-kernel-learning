import { actor } from "../shared/actor-continuity.js";
import {
  shot,
  closeShot,
  heading,
  note,
  pos,
  register,
  chip,
  wires,
  flow,
  reveal,
} from "../shared/scene-kit.js";
import {
  memory,
  memoryRows,
  showMemory,
  file,
  actualCode,
  commit,
  osShot,
  mountOS,
} from "../shared/os-components.js";
import {
  ready,
  entry,
  mainEntry,
  sample,
  object,
  mapping,
  osTrace as t,
} from "../models/os-basics.js";
const other = sample("main-entry", "second");
function process() {
  const s = osShot(
    shot(
      "space-instance",
      `<div class="camera">${heading("pi-head", 80, 100, "PROGRAM FILE ≠ PROCESS", "进程，是运行中的程序实例。")}${file("pi-file", 90, 315, "os-demo", "静态文件：代码与初始数据")}${chip("pi-cpu", 560, 300, { label: "CPU", subtitle: "OS 分配执行机会", width: 220 })}${memory("pi-view", 870, 255, memoryRows(ready, ["text", "data", "heap", "stack"]), { width: 470, title: `进程 PID ${ready.pid}`, direction: "自己的 VA 视图 ↑ · 不是另一台机器", cellHeight: 74, staged: true })}${note("pi-note", 95, 595, "进程 = 地址空间 + 可继续执行的上下文 + 资源", "“虚拟化”是让程序看到相对独立的执行环境，不是把它变成虚拟机。", { width: 1160 })}${wires(
        "pi-wires",
        [
          ["pi-launch", "M365 410H545", "blue", "启动"],
          ["pi-view-flow", "M790 410H855", "cyan", "视图"],
        ],
      )}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [{ at: 10, selector: "#pi-view", shown: true }],
      },
    ),
  );
  reveal(s.timeline, s.$("#pi-head"), 0.1);
  reveal(s.timeline, s.$("#pi-file"), 0.6);
  flow(s, "pi-launch", 1.5, 1.5);
  reveal(s.timeline, s.$("#pi-cpu"), 3.1);
  flow(s, "pi-view-flow", 4, 1.1);
  showMemory(s, "pi-view", 5.2, { staged: true, interval: 0.5 });
  reveal(s.timeline, s.$("#pi-note"), 8.5);
  return closeShot(s);
}
function distribution() {
  const stack = object(ready, "stack"),
    text = object(ready, "text");
  const s = osShot(
    shot(
      "space-distribution",
      `<div class="camera">${heading("vd-head", 80, 100, "ACTUAL LINUX USER VIRTUAL ADDRESSES", "把用户虚拟地址空间，竖起来看。")}${memory("vd-array", 785, 205, memoryRows(ready), { width: 560, title: `PID ${ready.pid} / 实测对象位置`, direction: "高地址 ↑ · 空洞与其他映射已折叠", staged: true, cellHeight: 56 })}${register("vd-top", 80, 255, "本次栈对象 / &local", stack.address, { width: 600 })}${register("vd-low", 80, 420, "本次代码 / &main", text.address, { width: 600 })}${note("vd-note", 80, 570, "栈通常向下；传统 brk 堆通常向上", "这里是逻辑分布，不按大小比例。mmap 地址、库的位置以及 malloc 来源都应看实际 maps。", { width: 620 })}</div>`,
      18,
      {
        stops: [11.5, 16.5],
        checks: [
          {
            at: 12,
            selector: "#vd-array-stack .array-address",
            text: stack.address,
          },
          { at: 12, selector: "#vd-array-bss .array-value", text: "0" },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#vd-head"), 0.1);
  showMemory(s, "vd-array", 0.6, { staged: true, interval: 0.85 });
  reveal(s.timeline, s.$("#vd-top"), 2);
  reveal(s.timeline, s.$("#vd-low"), 4);
  reveal(s.timeline, s.$("#vd-note"), 9);
  return closeShot(s);
}
function objects() {
  const rows = memoryRows(ready, ["data", "bss", "heap", "stack"]).map(
    (row) => ({ ...row, value: "—" }),
  );
  const defs = [
    ["data", 10, 3.3],
    ["bss", 0, 5.5],
    ["stack", 20, 7.7],
    ["heap", 30, 9.9],
  ];
  const routes = defs.map(([name, , at], i) => [
    `ob-${name}`,
    `M650 ${320 + (i > 1 ? 165 : 0)}C730 ${320 + (i > 1 ? 165 : 0)} 740 ${339 + rows.findIndex((r) => r.key === name) * 72} 835 ${339 + rows.findIndex((r) => r.key === name) * 72}`,
    "cyan",
    "定位",
  ]);
  const s = osShot(
    shot(
      "space-objects",
      `<div class="camera">${heading("ob-head", 80, 100, "C OBJECTS / ACTUAL VA AND VALUES", "同一程序里的对象，落在不同区域。")}${actualCode("ob-static", 80, 235, "os-math.c", ["int os_data = 10;", "int os_bss;"], { width: 550 })}${actualCode("ob-auto", 80, 400, "os-main.c", ["volatile int local = 20;", "os_heap = malloc(sizeof *os_heap);"], { width: 550 })}${memory("ob-array", 850, 250, rows, { width: 490, title: "实际对象 VA 与初始化后读值", direction: "VA ↑ · 指针变量与所指对象分开", cellHeight: 72 })}${note("ob-note", 80, 590, "指针变量，不等于它指向的内存", "本程序 os_heap 是 BSS 中的全局指针；它保存 malloc 对象的地址。", { width: 680 })}${wires("ob-wires", routes)}</div>`,
      16,
      {
        stops: [11.5, 14.5],
        checks: defs.map(([name, value]) => ({
          at: 12,
          selector: `#ob-array-${name} .array-value`,
          text: String(value),
        })),
      },
    ),
  );
  reveal(s.timeline, s.$("#ob-head"), 0.1);
  reveal(s.timeline, s.$("#ob-static"), 0.7);
  reveal(s.timeline, s.$("#ob-auto"), 1.2);
  showMemory(s, "ob-array", 1.8);
  defs.forEach(([name, value, at], i) => {
    flow(s, `ob-${name}`, at - 1.2, 1);
    commit(s, `#ob-array-${name} .array-value`, value, at);
  });
  reveal(s.timeline, s.$("#ob-note"), 11);
  return closeShot(s);
}
function virtual() {
  const rows = memoryRows(ready, ["private", "shared"]);
  const s = osShot(
    shot(
      "space-virtual",
      `<div class="camera">${heading("vm-head", 80, 100, "VIRTUAL ADDRESS ≠ PHYSICAL ADDRESS", "虚拟地址，是进程自己的坐标。")}${memory("vm-va", 80, 300, rows, { width: 520, title: `PID ${ready.pid} / 用户 VA`, direction: "VA ↑ · 这是进程看到的地址", cellHeight: 95 })}${memory(
        "vm-physical",
        890,
        300,
        [
          {
            label: "物理框 A（符号示意）",
            detail: "具体 PFN 未由本实验观测",
            value: "40",
          },
          {
            label: "物理框 S（符号示意）",
            detail: "框号与布局均非本机实测",
            value: "50",
          },
        ],
        {
          width: 450,
          title: "物理内存 / 机制解释",
          direction: "框号仅为示意 · 不是 VA",
          cellHeight: 95,
          space: "physical",
        },
      )}${note("vm-note", 135, 635, "要经过页表／MMU 翻译，才谈得上物理位置", "保留地址空间不等于所有页都驻留；未映射空洞也不等于占了一块 RAM。", { width: 1160 })}${wires(
        "vm-wires",
        [
          ["vm-map", "M620 400H875", "cyan", "翻译"],
          ["vm-shared-map", "M620 495H875", "cyan", "翻译"],
        ],
      )}</div>`,
      16,
      {
        stops: [10.5, 14.5],
        checks: [{ at: 11, selector: "#vm-physical", shown: true }],
      },
    ),
  );
  reveal(s.timeline, s.$("#vm-head"), 0.1);
  showMemory(s, "vm-va", 0.7);
  flow(s, "vm-map", 2, 2);
  showMemory(s, "vm-physical", 4.1);
  flow(s, "vm-shared-map", 5, 1.5);
  reveal(s.timeline, s.$("#vm-note"), 9);
  return closeShot(s);
}
function contexts() {
  const s = osShot(
    shot(
      "space-contexts",
      `<div class="camera">${heading("cx-head", 80, 100, "CPU VIRTUALIZATION / EXPLANATORY SWITCH", "保存现场，再让另一个进程继续。")}${register("cx-one", 80, 245, `PID ${mainEntry.pid} / 保存的 RIP`, mainEntry.registers.rip, { width: 420 })}${register("cx-two", 930, 245, `PID ${other.pid} / 保存的 RIP`, other.registers.rip, { width: 420 })}${chip("cx-cpu", 590, 300, { label: "CPU", subtitle: "一个逻辑处理器的示意", width: 250 })}${register("cx-live", 475, 590, "%rip / 当前示意上下文", "—", { width: 490 })}${note("cx-note", 80, 430, "地址空间与 CPU 上下文是两件事", "切换现场，不是复制整份 RAM。", { width: 420 })}${note("cx-scope", 930, 430, "这是机制演示，不是调度追踪", "两组现场取自两次独立 GDB 运行；没有测量真实上下文切换时刻。", { width: 420 })}${wires(
        "cx-wires",
        [
          ["cx-load-one", "M505 310C550 310 550 390 575 390", "cyan", "恢复"],
          ["cx-save-one", "M575 440C530 440 545 365 505 365", "blue", "保存"],
          ["cx-load-two", "M915 310C870 310 880 390 855 390", "cyan", "恢复"],
        ],
      )}</div>`,
      16,
      {
        stops: [6.5, 12.5, 14.5],
        checks: [
          {
            at: 13,
            selector: "#cx-live .reg-value",
            text: other.registers.rip,
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#cx-head"), 0.1);
  reveal(s.timeline, s.$("#cx-one"), 0.6);
  reveal(s.timeline, s.$("#cx-two"), 1.1);
  reveal(s.timeline, s.$("#cx-cpu"), 1.5);
  reveal(s.timeline, s.$("#cx-live"), 2);
  flow(s, "cx-load-one", 2.5, 1.5);
  commit(s, "#cx-live .reg-value", mainEntry.registers.rip, 4.1);
  flow(s, "cx-save-one", 6, 1.4);
  flow(s, "cx-load-two", 8, 1.5);
  commit(s, "#cx-live .reg-value", other.registers.rip, 9.6);
  reveal(s.timeline, s.$("#cx-note"), 10.5);
  reveal(s.timeline, s.$("#cx-scope"), 10.8);
  return closeShot(s);
}
mountOS({
  id: "os-process-space",
  title: "进程、虚拟化与内存空间分布",
  actorContinuity: [actor('one-process-address-space','array',[
    ['space-instance','#pi-view'],['space-distribution','#vd-array'],['space-objects','#ob-array'],['space-virtual','#vm-va']
  ])],
  labLocations: {
    source: 35,
    "layout-ready": 23,
    "main-entry": 70,
    "phase-2": 7,
  },
  scenes: [
    { title: "程序文件与进程实例", view: "PROGRAM ≠ PROCESS", build: process },
    {
      title: "竖向虚拟内存布局",
      view: "ACTUAL VA LAYOUT",
      build: distribution,
    },
    { title: "C 对象放在哪里", view: "ACTUAL OBJECTS", build: objects },
    {
      title: "虚拟与物理不是同一张图",
      view: "VIRTUAL / PHYSICAL MODEL",
      sourceKind: "teaching-model",
      build: virtual,
    },
    {
      title: "CPU 上下文的保存与恢复",
      view: "CPU VIRTUALIZATION MODEL",
      sourceKind: "teaching-model",
      build: contexts,
    },
  ],
});
