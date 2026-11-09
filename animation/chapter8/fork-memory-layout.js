import { mountExperimentalFilm as mountFilm } from "../shared/experimental-film.js";
import { actor } from "../shared/actor-continuity.js";
import {
  shot,
  closeShot,
  pos,
  heading,
  note,
  code,
  activeCode,
  register,
  number,
  page,
  windowBox,
  wires,
  flow,
  reveal,
  vanish,
  state,
} from "../shared/scene-kit.js";
import { cowTrace } from "../models/computer.js";
import { experiments } from "../models/experiments.js";
import { phaseOf } from "../models/validate-experiments.js";
const memoryRun = experiments["fork-memory"].runs.default;
const parentBefore = phaseOf(memoryRun, 1),
  childAfter = phaseOf(memoryRun, 3),
  parentAfter = phaseOf(memoryRun, 4);
const P = parentAfter.fields,
  C = childAfter.fields;
const source = experiments["fork-memory"].provenance.source.content.split("\n");
function realCode(id, x, y, terms, options = {}) {
  const indices = terms.map((term) =>
    source.findIndex((line) => line.includes(term)),
  );
  if (indices.some((i) => i < 0))
    throw new Error("fork C source fragment missing");
  return code(
    id,
    x,
    y,
    indices.map((i) => source[i].replace(/\s*\/\*.*?\*\//g, "").trim()),
    {
      ...options,
      title: "fork-memory.c / 真实源码节选",
      lineNumbers: indices.map((i) => i + 1),
    },
  );
}
const cow = cowTrace();
const regions = [
  ["用户栈", "0xF000 / RW"],
  ["共享匿名映射", "0xA000 / RW"],
  ["私有匿名映射", "0x8000 / RW"],
  ["堆", "0x7000 / RW"],
  [".data / .bss", "0x6000 / RW"],
  [".rodata", "0x5000 / R"],
  [".text", "0x4000 / RX"],
];
function virtualLayout() {
  const bars = regions
    .map(([name, address], i) =>
      pos(
        `virtual-region-${i}`,
        220,
        220 + i * 51,
        `<span>${name}</span><code>${address}</code>`,
        "memory-region",
        { width: 400 },
      ),
    )
    .join("");
  const s = shot(
    "memory-virtual-layout",
    `<div class="camera">${pos("virtual-parent", 220, 125, `<div class="eyebrow">PARENT / PID ${parentBefore.pid}</div><h2 class="scene-heading" style="margin-top:15px">一个<em>虚拟地址空间</em>。</h2>`, "", { visible: true })}${bars}
    ${realCode("virtual-code", 780, 240, ["*private_page = 40;", "*shared_page = 50;"], { width: 560 })}
    ${note("virtual-note", 780, 560, "区域按逻辑用途展开", `本图布局地址是模型；实机 private_page = ${P.privateAddress}。`, { width: 510 })}</div>`,
    7,
    {
      stops: [3.6, 5.8],
      checks: [{ at: 4, selector: "#virtual-region-6", shown: true }],
    },
  );
  regions.forEach((_, i) =>
    reveal(s.timeline, s.$(`#virtual-region-${i}`), 0.35 + i * 0.43, {
      y: -18,
    }),
  );
  reveal(s.timeline, s.$("#virtual-code"), 1.2);
  activeCode(s, 1, 2);
  activeCode(s, 1, 3.9);
  reveal(s.timeline, s.$("#virtual-note"), 4.7);
  return closeShot(s);
}
function independentTables() {
  const s = shot(
    "memory-fork-tables",
    `<div class="camera">
    ${windowBox("tables-parent", 140, 185, `父 PID ${parentBefore.pid}`, `<code>模型 VA 0x8000</code><small>实机 VA ${P.privateAddress}</small>`, { width: 340, visible: true })}
    ${windowBox("tables-child", 140, 463, `子 PID ${childAfter.pid}`, `<code>模型 VA 0x8000</code><small>实机 VA ${C.privateAddress}</small>`, { width: 340 })}
    ${pos("tables-PTP", 575, 205, '<strong>PT_P</strong><br><span class="soft-label">8000 → A</span><br><span class="tiny-note">PTE: RO / COW</span>', "relation-node", { width: 265 })}
    ${pos("tables-PTC", 575, 483, '<strong>PT_C</strong><br><span class="soft-label">8000 → A</span><br><span class="tiny-note">PTE: RO / COW</span>', "relation-node", { width: 265 })}
    ${page("tables-A", 1060, 295, "PHYSICAL PAGE A", "40")}${pos("tables-text", 980, 580, "只读代码 / 常量<br><strong>通常也可共享物理页</strong>", "anchored-note", { width: 330 })}
    ${heading("tables-heading", 180, 90, "FORK / ADDRESS SPACES", "页表各自独立，<em>物理页可以共享</em>。")}
    ${wires("tables-wires", [
      ["tables-p", "M495 265H558", "blue"],
      ["tables-c", "M495 543H558", "blue"],
      ["tables-p-A", "M855 270C980 270 940 350 1045 350", "cyan"],
      ["tables-c-A", "M855 550C990 550 950 420 1045 420", "cyan"],
    ])}</div>`,
    10,
    {
      stops: [3.5, 6, 8.4],
      checks: [
        { at: 1, selector: "#tables-child", shown: false },
        { at: 6.5, selector: "#tables-PTC", shown: true },
        { at: 6.5, selector: "#tables-A .page-value", text: "40" },
      ],
    },
  );
  reveal(s.timeline, s.$("#tables-heading"), 0.1);
  reveal(s.timeline, s.$("#tables-PTP"), 0.6);
  flow(s, "tables-p", 0.9, 0.8, { hide: false });
  reveal(s.timeline, s.$("#tables-child"), 2.1, { x: -25, y: 0 });
  reveal(s.timeline, s.$("#tables-PTC"), 2.6);
  flow(s, "tables-c", 2.9, 0.8, { hide: false });
  reveal(s.timeline, s.$("#tables-A"), 3.7, { y: 0, scale: 0.95 });
  flow(s, "tables-p-A", 4.1, 1.1, { hide: false });
  flow(s, "tables-c-A", 4.1, 1.1, { hide: false });
  reveal(s.timeline, s.$("#tables-text"), 6.6);
  state(s, "two-tables-one-physical-page", 5.3);
  return closeShot(s);
}
function copyOnWrite() {
  const copyRoutes = Array.from({ length: 12 }, (_, i) => [
    `cow-copy-${i}`,
    `M970 ${260 + (i % 4) * 20}C1060 ${260 + (i % 4) * 20} 970 470 1032 ${475 + (i % 4) * 18}`,
    "cyan",
  ]);
  const s = shot(
    "memory-copy-on-write",
    `<div class="camera">
    ${windowBox("cow-parent", 130, 170, "父 / PT_P", "<code>8000 → A</code><small>PTE: RO / COW</small>", { width: 370, visible: true })}
    ${windowBox("cow-child", 130, 430, "子 / PT_C", "<code>8000 → A</code><small>PTE: RO / COW</small>", { width: 370, visible: true })}
    ${page("cow-A", 750, 170, "A / 原物理页", "40", { visible: true })}${page("cow-B", 1040, 425, "B / 新分配整页", "…")}
    ${pos("cow-guard", 562, 462, "×", "cross-mark")}${pos("cow-mode", 575, 615, "同一 CPU / 内核处理 COW", "kernel-mode")}
    ${realCode("cow-code", 750, 640, ["*private_page = 400;"], { width: 570 })}
    ${note("cow-copy-note", 750, 390, "先复制整页，再改映射", "不是只复制一个 int。", { width: 290 })}
    ${pos("cow-phase", 570, 115, "区域允许写 / PTE 暂时只读", "eyebrow")}
    ${wires("cow-wires", [["cow-parent-A", "M515 245C610 245 650 245 733 245", "cyan"], ["cow-child-A", "M515 505C645 505 650 315 733 315", "cyan"], ["cow-write", "M350 610C500 610 490 487 552 487", "red", "400"], ["cow-remap", "M515 505C740 505 800 505 1023 505", "blue"], ...copyRoutes])}</div>`,
    14,
    {
      stops: [3.6, 8.5, 10.2, 12.3],
      checks: [
        { at: 2.8, selector: "#cow-B", shown: false },
        { at: 6.5, selector: "#cow-B .page-value", text: "…" },
        { at: 8.6, selector: "#cow-B .page-value", text: String(cow[3].B) },
        { at: 8.6, selector: "#cow-child code", text: "8000 → A" },
        { at: 10.5, selector: "#cow-child code", text: "8000 → B" },
        { at: 12.5, selector: "#cow-B .page-value", text: String(cow[4].B) },
        { at: 12.5, selector: "#cow-A .page-value", text: "40" },
      ],
    },
  );
  reveal(s.timeline, s.$("#cow-phase"), 0.1);
  flow(s, "cow-parent-A", 0.05, 0.65, { hide: false });
  flow(s, "cow-child-A", 0.05, 0.65, { hide: false });
  reveal(s.timeline, s.$("#cow-code"), 0.9);
  activeCode(s, 0, 1.3);
  flow(s, "cow-write", 1.5, 0.9);
  reveal(s.timeline, s.$("#cow-guard"), 2.5, { y: 0, scale: 0.7 });
  reveal(s.timeline, s.$("#cow-mode"), 3);
  state(s, "cow-write-protection-fault", 3);
  reveal(s.timeline, s.$("#cow-B"), 4.1, { y: 0, scale: 0.9 });
  reveal(s.timeline, s.$("#cow-copy-note"), 4.3);
  copyRoutes.forEach(([key], i) => flow(s, key, 4.5 + i * 0.23, 0.65));
  state(s, "copying-unmapped-B", 4.5);
  number(s.timeline, s.$("#cow-B .page-value"), cow[3].B, 8);
  state(s, "copy-complete-before-remap", 8);
  vanish(s.timeline, s.$("#cow-child-A-route"), 8.8, { y: 0 });
  flow(s, "cow-remap", 8.8, 1.1, { hide: false });
  number(s.timeline, s.$("#cow-child code"), "8000 → B", 10);
  number(s.timeline, s.$("#cow-child small"), "PTE: RW / 私有新页", 10);
  state(s, "child-remapped-to-B", 10);
  vanish(s.timeline, s.$("#cow-guard"), 10.5, { y: 0 });
  vanish(s.timeline, s.$("#cow-mode"), 10.5);
  number(s.timeline, s.$("#cow-B .page-value"), cow[4].B, 11.3);
  number(
    s.timeline,
    s.$("#cow-phase"),
    "返回用户态，重试原写入：父 40 / 子 400",
    11.3,
  );
  state(s, "write-retried", 11.3);
  return closeShot(s);
}
function explicitShared() {
  const s = shot(
    "memory-shared-map",
    `<div class="camera">${heading("shared-heading", 150, 115, "MAP_SHARED / ONE PHYSICAL PAGE", "显式共享，<em>写的是同一页</em>。")}
    ${windowBox("shared-parent", 150, 300, "父 / 模型 VA 0xA000", "<code>read: —</code><small>管道同步后再读取</small>", { width: 320 })}${windowBox("shared-child", 970, 300, "子 / 模型 VA 0xA000", "<code>write: 500</code><small>区域和 PTE 都允许写</small>", { width: 320 })}${page("shared-S", 610, 285, "PHYSICAL PAGE S", "50")}
    ${realCode("shared-code", 150, 555, ["*shared_page = 500;", "checkpoint(4);"], { width: 650 })}
    ${note("shared-note", 940, 565, "父也读到 500", `GDB 父进程也观测到 ${P.sharedValue}；物理页 S 为模型。`, { width: 350 })}
    ${wires("shared-wires", [
      ["shared-parent-map", "M485 355H590", "blue"],
      ["shared-child-write", "M955 355H850", "cyan", "500"],
      ["shared-parent-read", "M595 420C550 420 550 405 485 405", "cyan", "500"],
    ])}</div>`,
    10,
    {
      stops: [2.6, 5.1, 8.5],
      checks: [
        { at: 2, selector: "#shared-S .page-value", text: "50" },
        { at: 5.2, selector: "#shared-S .page-value", text: "500" },
        { at: 8, selector: "#shared-parent code", text: "read: 500" },
      ],
    },
  );
  reveal(s.timeline, s.$("#shared-heading"), 0.1);
  ["#shared-parent", "#shared-child", "#shared-S"].forEach((e, i) =>
    reveal(s.timeline, s.$(e), 0.6 + i * 0.3),
  );
  flow(s, "shared-parent-map", 1.6, 0.7, { hide: false });
  reveal(s.timeline, s.$("#shared-code"), 2.2);
  activeCode(s, 0, 2.5);
  flow(s, "shared-child-write", 3, 1.1);
  number(s.timeline, s.$("#shared-S .page-value"), 500, 4.2);
  state(s, "shared-written", 4.2);
  activeCode(s, 1, 5.1);
  flow(s, "shared-parent-read", 6, 1);
  number(s.timeline, s.$("#shared-parent code"), "read: 500", 7.1);
  reveal(s.timeline, s.$("#shared-note"), 7.3);
  return closeShot(s);
}
function kernelRecords() {
  const cols = [
    ["P", parentBefore.pid, 190],
    ["C", childAfter.pid, 865],
  ];
  const markup = cols
    .map(([who, pid, x]) =>
      [
        pos(
          `kernel-task-${who}`,
          x,
          200,
          `<strong>task_${who}</strong> · PID ${pid}`,
          "relation-node",
          { width: 360 },
        ),
        pos(
          `kernel-mm-${who}`,
          x,
          320,
          `<strong>mm_${who}</strong> · 独立地址空间`,
          "relation-node",
          { width: 360 },
        ),
        pos(
          `kernel-pt-${who}`,
          x,
          440,
          `<strong>pgd / PT_${who}</strong> · 用户页表`,
          "relation-node",
          { width: 360 },
        ),
        pos(
          `kernel-stack-${who}`,
          x,
          560,
          `<strong>KS_${who}</strong> · 独立内核栈`,
          "relation-node",
          { width: 360 },
        ),
        pos(
          `kernel-context-${who}`,
          x,
          630,
          `保存上下文 ctx_${who}`,
          "tiny-note",
        ),
      ].join(""),
    )
    .join("");
  const routes = cols.flatMap(([who, , x]) => [
    [`kernel-to-mm-${who}`, `M${x + 180} 253V303`, "blue"],
    [`kernel-to-pt-${who}`, `M${x + 180} 373V423`, "cyan"],
    [
      `kernel-to-stack-${who}`,
      `M${x + 365} 225H${x + 406}V587H${x + 377}`,
      "blue",
    ],
  ]);
  const s = shot(
    "memory-kernel-records",
    `<div class="camera">${heading("kernel-heading", 190, 95, "ONE KERNEL / TWO TASKS", "独立的记录，<em>不是两份 OS</em>。")}${markup}${page("kernel-S",610,285,"共享页 S / 机制背景","500")}${wires("kernel-record-wires", routes)}</div>`,
    8,
    {
      stops: [3.3, 6.5],
      checks: [
        { at: 6.5, selector: "#kernel-stack-P", shown: true },
        { at: 6.5, selector: "#kernel-stack-C", shown: true },
      ],
    },
  );
  reveal(s.timeline, s.$("#kernel-heading"), 0.1);
  reveal(s.timeline, s.$("#kernel-S"), 0.2);
  cols.forEach(([who], i) => {
    reveal(s.timeline, s.$(`#kernel-task-${who}`), 0.6 + i * 0.2);
    reveal(s.timeline, s.$(`#kernel-mm-${who}`), 1.7 + i * 0.2);
    flow(s, `kernel-to-mm-${who}`, 1.3 + i * 0.2, 0.7, { hide: false });
    reveal(s.timeline, s.$(`#kernel-pt-${who}`), 2.9 + i * 0.2);
    flow(s, `kernel-to-pt-${who}`, 2.5 + i * 0.2, 0.7, { hide: false });
    reveal(s.timeline, s.$(`#kernel-stack-${who}`), 4.5 + i * 0.2);
    flow(s, `kernel-to-stack-${who}`, 3.8 + i * 0.2, 1, { hide: false });
    reveal(s.timeline, s.$(`#kernel-context-${who}`), 5.3 + i * 0.2);
  });
  return closeShot(s);
}
function codeResults() {
  const results = [
    [".data", P.globalData, C.globalData],
    [".bss", P.globalBss, C.globalBss],
    ["用户栈 local", P.localValue, C.localValue],
    ["malloc 对象", P.heapValue, C.heapValue],
    ["MAP_PRIVATE", P.privateValue, C.privateValue],
    ["MAP_SHARED", P.sharedValue, C.sharedValue],
  ];
  const rows = results
    .map(
      ([name, p, c], i) =>
        `<div id="result-row-${i}" data-anim><span>${name}</span><span>${p}</span><span>${c}</span></div>`,
    )
    .join("");
  const s = shot(
    "memory-code-results",
    `<div class="camera">${heading("results-heading", 150, 100, "CODE / OBSERVED VALUES", "私有写入隔离，<em>共享写入可见</em>。")}
    ${realCode("results-code", 150, 265, ["if (fork_result == 0)", "*private_page = 400;", "*shared_page = 500;"], { width: 570 })}
    ${pos("results-table", 780, 250, `<div class="table-header"><span>对象</span><span>父读到</span><span>子写后</span></div>${rows}`, "result-table", { width: 550 })}
    ${page("results-S",150,455,"共享页 S / 仍为500","500")}${note("results-note", 420, 615, "完整程序有同步、错误处理和回收", "六行值来自GDB；页S仅作共享机制背景。", { width: 340 })}</div>`,
    8,
    {
      stops: [3.8, 6.8],
      checks: [
        { at: 6.6, selector: "#result-row-5 span:nth-child(2)", text: "500" },
      ],
    },
  );
  reveal(s.timeline, s.$("#results-heading"), 0.1);
  reveal(s.timeline, s.$("#results-S"), 0.2);
  reveal(s.timeline, s.$("#results-code"), 0.6);
  reveal(s.timeline, s.$("#results-table"), 0.8);
  results.forEach((_, i) =>
    reveal(s.timeline, s.$(`#result-row-${i}`), 1.2 + i * 0.65, {
      x: 12,
      y: 0,
    }),
  );
  activeCode(s, 1, 2.2);
  activeCode(s, 2, 4.2);
  reveal(s.timeline, s.$("#results-note"), 5.7);
  return closeShot(s);
}
function memorySummary() {
  const s = shot(
    "memory-summary",
    `<div class="camera">${heading("memory-summary-title", 170, 115, "VIRTUAL IS NOT PHYSICAL", "相同地址，<em>不一定是同一页</em>。")}
    ${windowBox("memory-private-result", 170, 300, "MAP_PRIVATE / VA 0x8000", "<code>父 → A = 40</code><br><code>子 → B = 400</code><small>写时复制后相互隔离</small>", { width: 450 })}
    ${page("memory-summary-S", 900, 300, "MAP_SHARED / S", "500")}${pos("memory-summary-parent", 770, 535, "父 VA A000", "soft-label")}${pos("memory-summary-child", 1115, 535, "子 VA A000", "soft-label")}
    ${note("memory-summary-note", 170, 625, "地址空间独立，不要求所有物理页独占", "页表、私有页、显式共享页，是三个不同层次。", { width: 950 })}
    ${wires("memory-summary-wires", [
      ["summary-shared-P", "M860 520C860 450 900 470 940 455", "cyan"],
      ["summary-shared-C", "M1185 520C1185 450 1120 470 1090 455", "cyan"],
    ])}</div>`,
    7,
    {
      stops: [3.4, 5.8],
      checks: [
        { at: 5.6, selector: "#memory-summary-S .page-value", text: "500" },
      ],
    },
  );
  reveal(s.timeline, s.$("#memory-summary-title"), 0.1);
  reveal(s.timeline, s.$("#memory-private-result"), 0.6, { x: -20, y: 0 });
  reveal(s.timeline, s.$("#memory-summary-S"), 1.7);
  reveal(s.timeline, s.$("#memory-summary-parent"), 2);
  reveal(s.timeline, s.$("#memory-summary-child"), 2.2);
  flow(s, "summary-shared-P", 2.5, 0.8, { hide: false });
  flow(s, "summary-shared-C", 2.5, 0.8, { hide: false });
  reveal(s.timeline, s.$("#memory-summary-note"), 4.3);
  return closeShot(s, { exit: false });
}
mountFilm(
  {
    id: "fork-memory-layout",
    chapter: 8,
    title: "父子进程的内存与内核布局",
    actorContinuity: [
      actor('cow-original-page-A','page',[['memory-fork-tables','#tables-A'],['memory-copy-on-write','#cow-A',{move:0.6}]]),
      actor('shared-page-S','page',[['memory-shared-map','#shared-S'],['memory-kernel-records','#kernel-S'],['memory-code-results','#results-S',{via:[610,455]}],['memory-summary','#memory-summary-S']]),
    ],
    model:
      "普通 Linux fork · VA / 页大小与布局为示意 · 共享读取有同步 · 非 PFN 实测",
    example: "./fork-memory-demo.c",
    scenes: [
      { title: "虚拟布局", detail: "先看父进程", build: virtualLayout },
      { title: "独立页表", detail: "物理页可以共享", build: independentTables },
      { title: "写时复制", detail: "分配 → 复制 → 改映射", build: copyOnWrite },
      { title: "显式共享", detail: "同一物理页 S", build: explicitShared },
      { title: "内核记录", detail: "task / mm / 内核栈", build: kernelRecords },
      { title: "代码与结果", detail: "同步后的观察", build: codeResults },
      {
        title: "独立与共享",
        detail: "相同 VA 不等于同页",
        build: memorySummary,
      },
    ],
  },
  "fork-memory",
);
