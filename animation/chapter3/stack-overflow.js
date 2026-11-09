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
  chip,
  page,
  draw,
  travel,
  wires,
  flow,
  reveal,
  vanish,
  state,
  hex,
} from "../shared/scene-kit.js";
import { experiments, measuredBuffer } from "../models/experiments.js";
const payloads = Object.fromEntries(
  Object.entries(measuredBuffer).map(([key, data]) => [key, data.writes]),
);
const bufferPayloads = Object.fromEntries(
  Object.entries(measuredBuffer).map(([key, data]) => [key, data.input]),
);
const bufferInitial = measuredBuffer.broken.initial;
const bufferSource =
  experiments["buffer-copy"].provenance.source.content.split("\n");
const actualRows = [
  "buf[copy_index] = input[copy_index];",
  "if (input[copy_index] == 0)",
].map((term) => bufferSource.findIndex((line) => line.includes(term)));
const addressRegister = (...args) =>
  register(...args).replace("register-cell", "register-cell measured-address");
const addressPage = (...args) =>
  page(...args).replace("memory-page", "memory-page measured-page");
const badReturnPC = measuredBuffer.broken.run.samples.find(
  (s) => s.event === "ret",
).registers.rip;
const badTarget = measuredBuffer.broken.returnAfter;
const regionNames = [
  "buf[8]",
  "对齐区 · 8B",
  "保存的 rbp · 8B",
  "返回地址 · 8B",
];
const stackBand = { left: 90, groupPitch: 326, cellWidth: 34, pitch: 38 };

function targetCell(id, index, width, initial, compact = false) {
  const inBuf = index < 8;
  const label = compact
    ? inBuf
      ? `[${index}]`
      : `+${index}`
    : inBuf
      ? `buf[${index}]`
      : "buf 外";
  return `<div id="${id}" class="buffer-target-cell ${inBuf ? "in-buf" : "outside-buf"}" data-offset="${index}" style="width:${width}px"><b>${initial === null ? "?" : hex(initial)}</b><small>${label}</small></div>`;
}
function band(
  prefix,
  { firstVisible = false, top = 400, labelsBelow = false } = {},
) {
  return regionNames
    .map((label, group) =>
      pos(
        `${prefix}-group-${group}`,
        stackBand.left + group * stackBand.groupPitch,
        top,
        `<div id="${prefix}-${group === 0 ? "target-label" : `region-${group}`}" class="buffer-region-title ${group === 0 && labelsBelow ? "buffer-target-label" : ""}">${group === 0 ? '<b class="buffer-destination-name">buf[8]</b><span>目标数组 · 只有 8 格</span>' : `<b>buf 外</b><span>${label}</span>`}</div>
    ${group === 0 ? `<div id="${prefix}-boundary" class="buffer-bounds" style="width:322px"></div>` : ""}
    <div class="buffer-target-row" style="gap:4px">${Array.from(
      { length: 8 },
      (_, j) => {
        const i = group * 8 + j;
        return targetCell(
          `${prefix}-byte-${i}`,
          i,
          stackBand.cellWidth,
          bufferInitial[i],
          true,
        );
      },
    ).join("")}</div>
    <div class="buffer-address">0x${hex(BigInt(measuredBuffer.broken.base) + BigInt(group * 8))}</div>`,
        `buffer-target-group ${labelsBelow ? "buffer-labels-below" : ""}`,
        { visible: group === 0 && firstVisible, width: 300 },
      ),
    )
    .join("");
}
function sourceBank(
  prefix,
  bytes,
  { left, top = 270, cellWidth, gap = 8, compact = false },
) {
  const cells = bytes
    .map(
      (value, i) =>
        `<div id="${prefix}-source-${i}" class="buffer-source-cell" data-source-index="${i}" style="width:${cellWidth}px"><i>${compact ? `[${i}]` : `input[${i}]`}</i><strong>${value === 0 ? "\\0" : value >= 32 && value <= 126 ? String.fromCharCode(value) : "·"}</strong><b class="buffer-source-hex">${hex(value)}</b></div>`,
    )
    .join("");
  return pos(
    `${prefix}-input`,
    left,
    top,
    `<div class="buffer-source-row" style="gap:${gap}px">${cells}</div>`,
    "buffer-source-bank",
    { width: bytes.length * (cellWidth + gap) - gap },
  );
}
function sourceLabel(id, left, top) {
  return pos(
    id,
    left,
    top,
    "<b>input</b><span>输入源 · 读取后仍保留原样</span>",
    "buffer-lane-label buffer-source-label",
  );
}
function copyCurrent(prefix, left, top, compact = false) {
  return pos(
    `${prefix}-current`,
    left,
    top,
    '<code class="buffer-reading">等待读取 input[0]</code><code class="buffer-writing">→ buf[0]</code>',
    `buffer-current ${compact ? "buffer-current-inline" : ""}`,
  );
}
function copyByte(s, prefix, r, i, at, duration, targetId) {
  const key = `${prefix}-write-${i}`,
    hit = at + duration + 0.08;
  // Keep measured commit instants, but do not draw overlapping byte copies:
  // the C loop is sequential and a selected GDB stop must still name byte i.
  const readAt = at + 0.22,
    flight = duration - 0.22;
  reveal(s.timeline, s.$(`#${key}-route`), readAt, { duration: 0.05, y: 0 });
  draw(s.timeline, s.$(`#${key}-path`), readAt, flight);
  travel(
    s.timeline,
    s.$(`#${key}-packet`),
    s.$(`#${key}-path`),
    readAt + 0.08,
    flight,
  );
  vanish(s.timeline, s.$(`#${key}-route`), at + duration + 0.14, {
    duration: 0.2,
    y: 0,
  });
  s.timeline.to(
    s.$(`#${prefix}-source-${i}`),
    { borderColor: "#86b1ff", backgroundColor: "#23385a", duration: 0.12 },
    readAt,
  );
  s.timeline.to(
    s.$(`#${prefix}-source-${i}`),
    { borderColor: "#45638e", backgroundColor: "#142033", duration: 0.2 },
    hit + 0.08,
  );
  number(
    s.timeline,
    s.$(`#${prefix}-current .buffer-reading`),
    `input[${i}] = ${hex(r.value)}`,
    readAt,
  );
  number(
    s.timeline,
    s.$(`#${prefix}-current .buffer-writing`),
    r.overflow ? `→ buf 外 +${i}` : `→ buf[${i}]`,
    readAt,
  );
  number(s.timeline, s.$(`#${targetId} b`), hex(r.value), hit);
  s.timeline.to(
    s.$(`#${targetId}`),
    {
      borderColor: r.overflow ? "#ff667b" : "#69e2f4",
      backgroundColor: r.overflow ? "#331b29" : "#193743",
      color: r.overflow ? "#ff98a7" : "#9df0fa",
      duration: 0.2,
    },
    hit,
  );
  s.timeline.set(
    s.$(`#${prefix}-current .buffer-writing`),
    { color: r.overflow ? "#ff98a7" : "#9df0fa" },
    hit,
  );
  if (r.overflow)
    s.timeline.to(
      s.$(`#${prefix}-boundary`),
      { borderColor: "#ff667b", duration: 0.2 },
      hit,
    );
  return hit;
}
function layout() {
  const s = shot(
    "buffer-layout",
    `<div class="camera">
    ${band("layout", { firstVisible: true })}
    ${heading("layout-heading", 90, 122, "DESTINATION / CAPACITY FIRST", "先认清：<em>buf 只有 8 字节</em>。")}
    ${code(
      "layout-code",
      90,
      565,
      actualRows.map((index) => bufferSource[index].trim()),
      {
        width: 700,
        title: "buffer-copy.c / 真实源码节选",
        lineNumbers: actualRows.map((index) => index + 1),
      },
    )}
    ${note("layout-direction", 880, 575, "右边三组，都不属于 buf", "连续写入向高地址推进；数组容量不会随输入增长。", { width: 480 })}
    ${pos("layout-size", 90, 540, "本图来自 broken 运行的写前快照；三个输入分别独立采集。", "buffer-unknown")}
  </div>`,
    8,
    {
      stops: [3.2, 6.5],
      checks: [{ at: 5.8, selector: "#layout-group-3", shown: true }],
    },
  );
  s.root.classList.add("buffer-copy-scene");
  reveal(s.timeline, s.$("#layout-heading"), 0.2);
  for (let i = 1; i < 4; i++)
    reveal(s.timeline, s.$(`#layout-group-${i}`), 0.7 + i * 1.05, {
      x: -20,
      y: 0,
    });
  reveal(s.timeline, s.$("#layout-code"), 1.1);
  activeCode(s, 1, 2.1);
  reveal(s.timeline, s.$("#layout-size"), 4.5);
  reveal(s.timeline, s.$("#layout-direction"), 5.1);
  return closeShot(s);
}
function smallCopy(key, duration) {
  const trace = payloads[key],
    edge = key === "edge",
    prefix = `copy-${key}`;
  const left = 300,
    cellWidth = 68,
    pitch = 76;
  const targetX = (i) => left + i * pitch + (i >= 8 ? 28 : 0);
  const routes = trace.map((r, i) => {
    const from = left + i * pitch + cellWidth / 2,
      to = targetX(i) + cellWidth / 2;
    return [
      `${prefix}-write-${i}`,
      `M${from} 362C${from} 390 ${to} 420 ${to} 466`,
      i >= 8 ? "red" : "cyan",
      hex(r.value),
    ];
  });
  const s = shot(
    `buffer-${key}`,
    `<div class="camera">
    ${heading(`${prefix}-title`, 170, 100, edge ? "ONE BYTE TOO MANY" : "SOURCE → DESTINATION", edge ? "input 多一字节，<em>buf 没有变大</em>。" : "复制 input，<em>只改写 buf</em>。")}
    ${sourceLabel(`${prefix}-source-label`, left, 210)}
    ${pos(`${prefix}-encoding`, left, 246, `字符 / HEX · \\0 = 00 · input @ ${measuredBuffer[key].inputAddress}`, "buffer-encoding")}
    ${sourceBank(prefix, bufferPayloads[key], { left, cellWidth })}
    ${pos(`${prefix}-target-label`, 90, 438, `<b>buf[8]</b><span>目标数组<br>固定 8 字节</span><code class="buffer-base-address">${measuredBuffer[key].base}</code>`, "buffer-lane-label buffer-target-label buffer-target-side", { width: 170 })}
    ${pos(`${prefix}-memory`, left, 475, `<div id="${prefix}-boundary" class="buffer-bounds" style="width:624px"></div><div class="buffer-target-row" style="gap:8px">${trace.map((r, i) => targetCell(`${prefix}-cell-${i}`, i, cellWidth, measuredBuffer[key].initial[i])).join("")}</div>`, "buffer-destination", { width: 704 })}
    ${pos(`${prefix}-call`, 1030, 240, '<code><b class="buffer-destination-name">buf</b> ← <b class="buffer-source-name">input</b></code><span><b class="buffer-destination-name">目标</b> ← <b class="buffer-source-name">源</b></span>', "buffer-copy-call", { width: 280 })}
    ${copyCurrent(prefix, 1030, 375)}
    ${pos(`${prefix}-unknown`, left, 585, "CC 是程序明确初始化的写前值；每格均为 GDB 实测。", "buffer-unknown")}
    ${addressRegister(`${prefix}-saved`, 1030, 610, "栈内返回地址", measuredBuffer[key].returnBefore, { width: 280 })}
    ${note(`${prefix}-result`, 210, 635, edge ? `第 9 字节 00：写到框外 ${payloads.edge[8].address}` : "input 保持原样；buf 中得到同一组字节", edge ? "buf[0..7] 已写满；越界的是复制的目标位置。" : "结尾 00 也占一格，正好写满 8 格。", { red: edge, width: 730 })}
    ${pos(`${prefix}-return`, 1030, 730, edge ? "本次确实返回，但已越界" : "GDB：本次正常返回", "local-note")}
    ${wires(`${prefix}-wires`, routes, {packetRadius:14})}
  </div>`,
    duration,
    {
      stops: edge ? [5.6, 6.7] : [5.8, 8.3],
      checks: [
        {
          at: edge ? 6 : 6.1,
          selector: `#${prefix}-cell-${trace.length - 1} b`,
          text: "00",
        },
        {
          at: edge ? 6 : 8,
          selector: `#${prefix}-saved .reg-value`,
          text: measuredBuffer[key].returnBefore,
        },
        { at: 0.2, selector: `#${prefix}-result`, shown: false },
        {
          at: 6,
          selector: `#${prefix}-source-${trace.length - 1} .buffer-source-hex`,
          text: "00",
        },
        { at: 6, selector: `#${prefix}-target-label b`, text: "buf[8]" },
      ],
    },
  );
  s.root.classList.add("buffer-copy-scene");
  reveal(s.timeline, s.$(`#${prefix}-title`), 0.05);
  for (const part of [
    "source-label",
    "encoding",
    "input",
    "target-label",
    "memory",
    "unknown",
    "call",
    "current",
  ])
    reveal(s.timeline, s.$(`#${prefix}-${part}`), 0.1, {
      y: 0,
      duration: 0.45,
    });
  reveal(s.timeline, s.$(`#${prefix}-saved`), 0.6);
  trace.forEach((r, i) => {
    const hit = copyByte(
      s,
      prefix,
      r,
      i,
      0.65 + i * 0.48,
      0.58,
      `${prefix}-cell-${i}`,
    );
    if (r.overflow) state(s, "first-illegal-write", hit);
  });
  reveal(s.timeline, s.$(`#${prefix}-result`), edge ? 5.2 : 5.5);
  reveal(s.timeline, s.$(`#${prefix}-return`), edge ? 6 : 7.4);
  return closeShot(s);
}
function corruption() {
  const trace = payloads.broken;
  const routes = trace.map((r, i) => {
    const from = 156 + i * 42 + 18;
    const to =
      stackBand.left +
      Math.floor(i / 8) * stackBand.groupPitch +
      (i % 8) * stackBand.pitch +
      stackBand.cellWidth / 2;
    return [
      `broken-write-${i}`,
      `M${from} 362C${from} 390 ${to} 401 ${to} 431`,
      i >= 8 ? "red" : "cyan",
      hex(r.value),
    ];
  });
  const s = shot(
    "buffer-corruption",
    `<div class="camera">
    ${heading("broken-title", 90, 96, "COPY SOURCE → STACK DESTINATION", "input 不变，<em>buf 被逐字节改写</em>。")}
    ${copyCurrent("broken", 90, 186, true)}
    ${sourceLabel("broken-source-label", 90, 225)}
    ${sourceBank("broken", bufferPayloads.broken, { left: 156, cellWidth: 36, gap: 6, compact: true })}
    ${band("broken", { top: 440, labelsBelow: true })}
    ${register("broken-count", 1120, 105, "已写字节", "0 / 27", { width: 220 })}
    ${addressRegister("broken-return-value", 1030, 630, "栈内地址快照 · 尚未 ret", measuredBuffer.broken.returnBefore, { width: 320 })}
    ${pos("broken-unknown", 90, 620, "CC 是真实初始化值；下方是这次 GDB 运行的栈地址。", "buffer-unknown")}
    ${note("broken-first", 90, 655, "第一次非法写：input[8] → buf 外 +8", `写到 ${payloads.broken[8].address}；还没有碰到返回地址。`, { red: true, width: 850 })}
    ${note("broken-rbp", 90, 655, "越过对齐区，覆盖保存的旧 rbp", "这些字节仍来自上方 input，不是 buf 自己变长。", { red: true, width: 850 })}
    ${note("broken-ret-note", 90, 655, "input 中的 AD DE 00，改写了返回地址", `低字节改写后为 ${badTarget}；不是把整个地址变成 0xDEAD。`, { red: true, width: 850 })}
    ${wires("broken-wires", routes, {packetRadius:14})}
  </div>`,
    16,
    {
      stops: [4.8, 8.2, 12.6, 14.5],
      checks: [
        { at: 4.2, selector: "#broken-first", shown: false },
        { at: 5, selector: "#broken-byte-8 b", text: "41" },
        {
          at: 10.5,
          selector: "#broken-return-value .reg-value",
          text: measuredBuffer.broken.returnBefore,
        },
        {
          at: 11.05,
          selector: "#broken-return-value .reg-value",
          text: `0x${hex(payloads.broken[24].returned)}`,
        },
        {
          at: 12.5,
          selector: "#broken-return-value .reg-value",
          text: badTarget,
        },
        {
          at: 13,
          selector: "#broken-source-24 .buffer-source-hex",
          text: "AD",
        },
        {
          at: 13,
          selector: "#broken-source-25 .buffer-source-hex",
          text: "DE",
        },
      ],
    },
  );
  s.root.classList.add("buffer-copy-scene");
  reveal(s.timeline, s.$("#broken-title"), 0.1);
  reveal(s.timeline, s.$("#broken-count"), 0.1);
  reveal(s.timeline, s.$("#broken-return-value"), 0.4);
  for (const id of [
    "broken-source-label",
    "broken-input",
    "broken-target-label",
    "broken-current",
    "broken-unknown",
  ])
    reveal(s.timeline, s.$(`#${id}`), 0.1, { y: 0, duration: 0.45 });
  for (let i = 0; i < 4; i++)
    reveal(s.timeline, s.$(`#broken-group-${i}`), 0.1 + i * 0.08, { y: 0 });
  trace.forEach((r, i) => {
    const hit = copyByte(
      s,
      "broken",
      r,
      i,
      0.6 + i * 0.4,
      0.5,
      `broken-byte-${i}`,
    );
    number(s.timeline, s.$("#broken-count .reg-value"), `${i + 1} / 27`, hit);
    number(
      s.timeline,
      s.$("#broken-return-value .reg-value"),
      `0x${hex(r.returned, 4)}`,
      hit,
    );
    if (i === 8) {
      reveal(s.timeline, s.$("#broken-first"), hit);
      state(s, "overflow-before-return-address", hit);
    }
    if (i === 16) {
      vanish(s.timeline, s.$("#broken-first"), hit - 0.4);
      reveal(s.timeline, s.$("#broken-rbp"), hit);
    }
    if (i === 24) {
      vanish(s.timeline, s.$("#broken-rbp"), hit - 0.4);
      reveal(s.timeline, s.$("#broken-ret-note"), hit);
      state(s, "saved-return-corrupted", hit);
    }
  });
  return closeShot(s);
}
function useReturn() {
  const target = `0x${hex(traceLast().returned, 4)}`;
  const s = shot(
    "buffer-ret-use",
    `<div class="camera">${addressRegister("damaged-address", 175, 270, "GDB / 保存的返回槽内地址", target, { width: 320, visible: true })}${chip("ret-cpu", 610, 260, { subtitle: "用户态 · 执行 ret" })}${addressRegister("damaged-rip", 1040, 310, "%rip · 当前执行位置", badReturnPC, { width: 280 })}
    ${code("ret-code", 170, 100, ["(gdb) x/i $rip", measuredBuffer.broken.run.samples.find((s) => s.event === "ret").commands.find((c) => c.command === "x/i $rip").output], { width: 1000, title: "GDB / 实际 ret 指令停点" })}
    ${note("ret-frame-note", 170, 530, "旧 rbp 也损坏，但不等于立刻取指异常", "本例 ret 从当前栈顶读取返回地址。", { width: 620 })}${note("ret-use-note", 965, 535, "现在才改变控制流", "写入与使用，发生在不同时间。", { red: true, width: 350 })}
    ${wires("ret-use-wires", [
      ["ret-read", "M460 355C520 355 540 365 600 365", "blue", "RA"],
      ["ret-set-rip", "M880 365C955 365 960 360 1020 360", "blue", "RA"],
    ])}</div>`,
    10,
    {
      stops: [3.1, 6.4, 8.4],
      checks: [
        { at: 3, selector: "#damaged-rip .reg-value", text: badReturnPC },
        { at: 7.4, selector: "#damaged-rip .reg-value", text: badTarget },
      ],
    },
  );
  reveal(s.timeline, s.$("#ret-cpu"), 0.2);
  reveal(s.timeline, s.$("#damaged-rip"), 0.3);
  reveal(s.timeline, s.$("#ret-code"), 0.5);
  activeCode(s, 0, 1);
  reveal(s.timeline, s.$("#ret-frame-note"), 1.4);
  activeCode(s, 1, 3.6);
  flow(s, "ret-read", 3.5, 1.1);
  flow(s, "ret-set-rip", 4.9, 1.1);
  number(s.timeline, s.$("#damaged-rip .reg-value"), target, 6.1);
  reveal(s.timeline, s.$("#ret-use-note"), 6.4);
  state(s, "ret-used-corrupted-address", 6.1);
  return closeShot(s);
}
function traceLast() {
  return payloads.broken.at(-1);
}
function fetchFault() {
  const s = shot(
    "buffer-fetch-fault",
    `<div class="camera">${chip("fault-cpu", 270, 270, { subtitle: "同一颗 CPU" })}${addressRegister("fault-rip", 220, 150, "%rip", badTarget, { width: 360, visible: true })}
    ${page("fault-code-page", 1070, 285, badTarget, "未映射", { width: 250 })}${pos("fault-cross", 965, 349, "×", "cross-mark")}
    ${pos("fault-kernel-mode", 285, 560, "内核态 / 异常处理", "kernel-mode")}${heading("fault-heading", 730, 115, "INSTRUCTION FETCH FAULT", "真正失败在<em>取指</em>。")}
    ${note("fault-note", 750, 535, "实测：SIGSEGV / si_code = 1", `si_addr = ${badTarget}；先写坏地址，ret 使用后才失败。`, { red: true, width: 560 })}
    ${wires("fault-wires", [["fault-fetch", "M540 395C720 395 790 378 955 378", "red"]])}</div>`,
    8,
    {
      stops: [3.6, 5.7, 7.1],
      checks: [
        { at: 1.8, selector: "#fault-kernel-mode", shown: false },
        { at: 5.8, selector: "#fault-kernel-mode", shown: true },
      ],
    },
  );
  reveal(s.timeline, s.$("#fault-cpu"), 0.1);
  reveal(s.timeline, s.$("#fault-heading"), 1.3);
  reveal(s.timeline, s.$("#fault-code-page"), 1.2);
  flow(s, "fault-fetch", 1.7, 1.4, { hide: false });
  reveal(s.timeline, s.$("#fault-cross"), 3.2, { y: 0, scale: 0.7 });
  reveal(s.timeline, s.$("#fault-kernel-mode"), 4.3);
  reveal(s.timeline, s.$("#fault-note"), 4.5);
  state(s, "instruction-fetch-fault", 4.3);
  return closeShot(s, { exit: false });
}
mountFilm(
  {
    id: "stack-overflow",
    chapter: 3,
    title: "栈缓冲区越界写",
    actorContinuity: [
      actor('damaged-return-slot','register',[['buffer-corruption','#broken-return-value'],['buffer-ret-use','#damaged-address',{via:[175,630]}]]),
      actor('same-cpu-ret-fetch','chip',[['buffer-ret-use','#ret-cpu'],['buffer-fetch-fault','#fault-cpu']]),
      actor('ret-to-fault-rip','register',[['buffer-ret-use','#damaged-rip'],['buffer-fetch-fault','#fault-rip',{via:[1040,150]}]]),
    ],
    model:
      "x86-64 / 小端 · 预设教学布局与越界结果 · 非真实漏洞 · 不同于递归栈耗尽",
    scenes: [
      { title: "内存布局", detail: "数组不是整个栈帧", build: layout },
      {
        title: "正常输入",
        detail: "7 字符 + 00",
        build: () => smallCopy("safe", 10),
      },
      {
        title: "多一个字节",
        detail: "结尾 00 也越界",
        build: () => smallCopy("edge", 8),
      },
      { title: "逐字节破坏", detail: "从 buf 写到返回地址", build: corruption },
      { title: "ret 使用地址", detail: "损坏 ≠ 立即跳转", build: useReturn },
      { title: "取指异常", detail: "同一 CPU 转入内核", build: fetchFault },
    ],
  },
  "buffer-copy",
);
