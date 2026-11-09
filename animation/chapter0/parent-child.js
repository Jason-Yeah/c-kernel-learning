import { actor } from "../shared/actor-continuity.js";
import {
  shot,
  closeShot,
  heading,
  note,
  pos,
  register,
  wires,
  flow,
  reveal,
} from "../shared/scene-kit.js";
import {
  memory,
  memoryRows,
  showMemory,
  actualCode,
  commit,
  osShot,
  mountOS,
} from "../shared/os-components.js";
import {
  parent,
  child,
  changed,
  parentAfter,
  object,
} from "../models/os-basics.js";
const names = ["data", "bss", "heap", "private", "shared", "stack"];
function before() {
  const s = osShot(
    shot(
      "family-parent",
      `<div class="camera">${heading("fp-head", 80, 100, "BEFORE FORK / REAL PARENT", "先只有父进程，和它自己的地址空间。")}${actualCode("fp-c", 80, 255, "os-main.c", ["volatile int local = 20;", "*os_heap = 30;", "*os_private = 40;", "*os_shared = 50;"], { width: 590 })}${memory("fp-memory", 900, 230, memoryRows(parent, names), { width: 450, title: `父 PID ${parent.pid}`, direction: "实测 VA ↑ · 每个对象各有地址", cellHeight: 58, staged: true })}${note("fp-note", 80, 545, "data=10，bss=0；局部、堆、mmap 各自初始化", "这里尚未显示子进程。后面会对照同一次 fork 的父子记录。", { width: 660 })}${wires("fp-wires", [["fp-init", "M685 400H885", "cyan", "初值"]])}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [
          { at: 10, selector: "#fp-memory-private .array-value", text: "40" },
          { at: 10, selector: "#fp-memory-shared .array-value", text: "50" },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#fp-head"), 0.1);
  reveal(s.timeline, s.$("#fp-c"), 0.7);
  flow(s, "fp-init", 2, 1.5);
  showMemory(s, "fp-memory", 3.6, { staged: true, interval: 0.45 });
  reveal(s.timeline, s.$("#fp-note"), 8.5);
  return closeShot(s);
}
function fork() {
  const s = osShot(
    shot(
      "family-fork",
      `<div class="camera">${heading("ff-head", 80, 100, "FORK / TWO INDEPENDENT VIEWS", "地址相同，但现在是两个进程。")}${memory("ff-parent", 80, 250, memoryRows(parent, names), { width: 470, title: `父 PID ${parent.pid}`, direction: "父虚拟地址 ↑ · 原视图继续存在", cellHeight: 58 })}${memory("ff-child", 890, 250, memoryRows(child, names), { width: 470, title: `子 PID ${child.pid}`, direction: "子虚拟地址 ↑ · 继承的相同 VA", cellHeight: 58 })}${pos("ff-call", 620, 270, "fork()", "os-mid", { width: 200 })}${register("ff-pr", 605, 450, "父返回", "—", { width: 220 })}${register("ff-cr", 605, 565, "子返回", "—", { width: 220 })}${wires("ff-wires", [["ff-split", "M565 375H875", "cyan", "分叉"]])}</div>`,
      16,
      {
        stops: [10.5, 14.5],
        checks: [
          { at: 11, selector: "#ff-pr .reg-value", text: String(child.pid) },
          { at: 11, selector: "#ff-cr .reg-value", text: "0" },
          {
            at: 11,
            selector: "#ff-child-private .array-address",
            text: object(parent, "private").address,
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#ff-head"), 0.1);
  showMemory(s, "ff-parent", 0.7);
  reveal(s.timeline, s.$("#ff-call"), 1.3);
  flow(s, "ff-split", 2.4, 2.2);
  showMemory(s, "ff-child", 4.7);
  reveal(s.timeline, s.$("#ff-pr"), 6);
  commit(s, "#ff-pr .reg-value", child.pid, 6.1);
  reveal(s.timeline, s.$("#ff-cr"), 7);
  commit(s, "#ff-cr .reg-value", 0, 7.1);
  return closeShot(s);
}
function writes() {
  const rows = memoryRows(child, names),
    keys = ["data", "bss", "stack", "heap", "private", "shared"];
  const routes = keys.map((name, i) => {
    const y = 250 + 53 + rows.findIndex((r) => r.key === name) * 58 + 29;
    return [`fw-write-${name}`, `M850 ${y}H885`, "cyan", "写"];
  });
  const sharedY =
    250 + 53 + rows.findIndex((r) => r.key === "shared") * 58 + 29;
  routes.push([
    "fw-sync",
    `M885 ${sharedY}H875V735H55V${sharedY}H65`,
    "blue",
    "同步",
  ]);
  const s = osShot(
    shot(
      "family-writes",
      `<div class="camera">${heading("fw-head", 80, 100, "PRIVATE ISOLATION / SHARED VISIBILITY", "子进程修改后，父读到了什么？")}${memory("fw-parent", 80, 250, memoryRows(parent, names), { width: 460, title: `父 PID ${parent.pid}`, direction: "父 VA ↑ · 管道同步后读取", cellHeight: 58 })}${memory("fw-child", 900, 250, rows, { width: 460, title: `子 PID ${child.pid}`, direction: "子 VA ↑ · 实际写入", cellHeight: 58 })}${actualCode("fw-code", 560, 560, "os-main.c", ["*os_private = 400;", "*os_shared = 500;"], { width: 320 })}${note("fw-note", 570, 280, "私有隔离／共享可见", "私有：父40、子400。共享：同步后都读500。管道仅传控制字节，不复制整数500。", { width: 300 })}${wires("fw-wires", routes)}</div>`,
      20,
      {
        stops: [10.5, 15.5, 18.5],
        checks: [
          { at: 16, selector: "#fw-parent-private .array-value", text: "40" },
          { at: 16, selector: "#fw-child-private .array-value", text: "400" },
          { at: 16, selector: "#fw-parent-shared .array-value", text: "500" },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#fw-head"), 0.1);
  showMemory(s, "fw-parent", 0.5);
  showMemory(s, "fw-child", 0.8);
  reveal(s.timeline, s.$("#fw-code"), 7.2);
  keys.forEach((name, i) => {
    const at = 1.8 + i * 0.85;
    flow(s, `fw-write-${name}`, at, 0.55);
    commit(
      s,
      `#fw-child-${name} .array-value`,
      object(changed, name).value,
      at + 0.64,
    );
  });
  flow(s, "fw-sync", 8, 2.2);
  commit(
    s,
    "#fw-parent-shared .array-value",
    object(parentAfter, "shared").value,
    10.3,
  );
  reveal(s.timeline, s.$("#fw-note"), 12);
  return closeShot(s);
}
function explain() {
  const routes = [
    ["fe-parent", "M505 386H540", "blue", "映射"],
    ["fe-child", "M935 386C920 386 920 481 900 481", "cyan", "映射"],
    ["fe-shared-one", "M505 481C530 481 520 576 540 576", "blue", "共享"],
    ["fe-shared-two", "M935 481C920 481 925 576 900 576", "cyan", "共享"],
  ];
  const s = osShot(
    shot(
      "family-physical-model",
      `<div class="camera">${heading("fe-head", 80, 100, "MECHANISM MODEL / NOT PFN EVIDENCE", "同 VA，并不能证明同一物理页。")}${memory("fe-parent", 80, 285, memoryRows(parentAfter, ["private", "shared"]), { width: 410, title: "父 / 实测结果", direction: "VA ↑ · private40 / shared500", cellHeight: 95 })}${memory(
        "fe-physical",
        555,
        285,
        [
          { label: "父私有页 A", detail: "框号只作机制示意", value: "40" },
          { label: "子私有页 B", detail: "写后独立的机制示意", value: "400" },
          { label: "共享页 S", detail: "双方引用同页的机制示意", value: "500" },
        ],
        {
          width: 330,
          title: "物理框 / 机制图",
          direction: "非实际 PFN · 非比例布局",
          cellHeight: 95,
          space: "physical",
        },
      )}${memory("fe-child", 950, 285, memoryRows(changed, ["private", "shared"]), { width: 410, title: "子 / 实测结果", direction: "VA ↑ · private400 / shared500", cellHeight: 95 })}${note("fe-note", 170, 665, "实测了隔离与共享值；COW 复制／PTE 是机制解释", "普通 GDB 的同 VA、maps 或值对照，不足以直接证明物理页框。", { width: 1110 })}${wires("fe-wires", routes)}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [
          { at: 10, selector: "#fe-parent-shared .array-value", text: "500" },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#fe-head"), 0.1);
  showMemory(s, "fe-parent", 0.7);
  showMemory(s, "fe-child", 1.2);
  showMemory(s, "fe-physical", 2);
  routes.forEach(([key], i) => flow(s, key, 3 + i * 1.2, 0.7));
  reveal(s.timeline, s.$("#fe-note"), 8.5);
  return closeShot(s);
}
mountOS({
  id: "os-parent-child",
  title: "父子进程：内存地址与私有／共享",
  actorContinuity: [
    actor('family-parent-memory', 'array', [['family-parent','#fp-memory'],['family-fork','#ff-parent'],['family-writes','#fw-parent'],['family-physical-model','#fe-parent']]),
    actor('family-child-memory', 'array', [['family-fork','#ff-child'],['family-writes','#fw-child'],['family-physical-model','#fe-child']]),
  ],
  labLocations: {
    source: 3,
    "phase-2": 9,
    "phase-3": 24,
    "phase-4": 41,
    "phase-5": 47,
  },
  scenes: [
    { title: "fork 前的父地址空间", view: "ACTUAL PARENT", build: before },
    { title: "父子两列相同 VA", view: "ACTUAL FORK / SAME VA", build: fork },
    { title: "私有隔离与共享值", view: "ACTUAL SIX OBJECTS", build: writes },
    {
      title: "不要把 VA 当物理页",
      view: "PHYSICAL MODEL",
      sourceKind: "teaching-model",
      build: explain,
    },
  ],
});
