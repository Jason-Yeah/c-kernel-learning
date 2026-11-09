import { mountFilm } from "../shared/player.js";
import { actor } from "../shared/actor-continuity.js";
import {
  shot,
  closeShot,
  heading,
  note,
  pos,
  code,
  register,
  chip,
  wires,
  flow,
  reveal,
  number,
  escape,
} from "../shared/scene-kit.js";
import { memory, showMemory } from "../shared/vertical-memory.js";
import {
  cycleTrace as t,
  cycleSummary,
  at,
  load,
  loaded,
  add,
  added,
  store,
  stored,
  ret,
  returned,
  hx,
  byte,
  byteString,
  loadEA,
  loadDisp,
  storeEA,
  sourcePart,
  inputRows,
  codeRows,
  experiment,
} from "../models/instruction-cycle.js";
const set = (s, selector, value, time) =>
  number(s.timeline, s.$(selector), String(value), time);
const show = (s, selector, time) =>
  reveal(s.timeline, s.$(selector), time, { y: 0 });
const make = (s) => {
  s.root.classList.add("instruction-scene");
  return s;
};
const asm = (s) => s.instruction.asm.split("#")[0].trim();
function intro() {
  const source = sourcePart([
    "volatile int inputs[2]",
    "volatile int observed;",
    "int left = inputs[0];",
    "int right = inputs[1];",
    "int result = left + right;",
    "observed = result;",
    "return result;",
  ]);
  const rows = [
    {
      key: "out",
      label: "observed / 写入前",
      address: load.output.address,
      value: load.output.value,
      role: "target-row",
    },
    ...inputRows(),
  ];
  const s = make(
    shot(
      "cycle-program",
      `<div class="camera">${heading("ic-head", 80, 100, "REAL C → REAL INSTRUCTIONS", "这次，看一条指令怎样推进计算。")}${code("ic-source", 80, 235, source.lines, { width: 690, title: "instruction-cycle.c / 真实源码节选", lineNumbers: source.numbers })}${memory("ic-data", 930, 250, rows, { width: 420, title: `PID ${load.pid} / 已初始化数据`, direction: "实测用户 VA ↑ · 非物理 RAM", cellHeight: 86, staged: true })}${note("ic-note", 80, 620, "一行 C，不一定只对应一条机器指令", "将放大实际 MOV、ADD、STORE 与 RET；完整程序及全部真实单步在下方实验台。", { width: 1220 })}</div>`,
      12,
      {
        stops: [8.5, 10.5],
        checks: [
          { at: 9, selector: "#ic-data-out .array-value", text: "0" },
          { at: 9, selector: "#ic-data-0 .array-value", text: "3" },
        ],
      },
    ),
  );
  show(s, "#ic-head", 0.1);
  show(s, "#ic-source", 0.7);
  showMemory(s, "ic-data", 1.4, { staged: true, interval: 0.65 });
  show(s, "#ic-note", 6.7);
  return closeShot(s);
}
function fetchInstruction() {
  const routes = load.instruction.bytes.map((v, i) => [
    `fetch-${i}`,
    `M605 ${307 + (5 - i) * 58}C700 ${307 + (5 - i) * 58} 710 390 800 390`,
    "blue",
    byte(v),
  ]);
  const s = make(
    shot(
      "cycle-fetch",
      `<div class="camera">${heading("if-head", 80, 100, "FETCH / INSTRUCTION BYTES", "RIP 指向代码，先取指令字节。")}${memory("if-code", 90, 225, codeRows, { width: 500, title: "代码内存 / 此条真实 MOV", direction: "VA ↑ · 最高地址在上 · 源字节不删除", cellHeight: 58 })}${register("if-rip", 780, 225, "%rip / 当前指令地址", load.registers.rip, { width: 540 })}${chip("if-cpu", 820, 340, { label: "取指", subtitle: "控制过程为机制示意", width: 230 })}${register("if-bytes", 770, 590, "取出的字节 / 非公开架构寄存器", "—", { width: 570 })}${note("if-note", 80, 680, "这条实际指令长6字节，不是固定1字节", "这里演示取指含义；GDB测的是边界字节与状态，不是指令总线、Cache/TLB或真实时钟。", { width: 1260 })}${wires("if-wires", routes)}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [
          {
            at: 10,
            selector: "#if-bytes .reg-value",
            text: byteString(load.instruction.bytes),
          },
          {
            at: 10,
            selector: "#if-code-0 .array-value",
            text: byte(load.instruction.bytes[0]),
          },
        ],
      },
    ),
  );
  show(s, "#if-head", 0.1);
  showMemory(s, "if-code", 0.6);
  show(s, "#if-rip", 0.8);
  show(s, "#if-cpu", 1);
  show(s, "#if-bytes", 1.4);
  routes.forEach(([key], i) => {
    const time = 2 + i * 0.7;
    flow(s, key, time, 0.65);
    set(
      s,
      "#if-bytes .reg-value",
      byteString(load.instruction.bytes.slice(0, i + 1)),
      time + 0.74,
    );
  });
  show(s, "#if-note", 8.5);
  return closeShot(s);
}
function decode() {
  const bytes = load.instruction.bytes;
  const s = make(
    shot(
      "cycle-decode",
      `<div class="camera">${heading("id-head", 80, 100, "DECODE / SOFTWARE-VERIFIABLE FIELDS", "字节，怎样说明“从哪里读到哪里”？")}${memory("id-code",90,225,codeRows,{width:500,title:"代码内存 / 同一条 MOV",direction:"VA ↑ · 沿用刚取过的原始字节",cellHeight:58})}${register("id-rip",650,210,"%rip / 当前控制位置",load.registers.rip,{width:700})}${register("id-opcode", 650, 330, "opcode / 读r32", byte(bytes[0]), { width: 220 })}${register("id-modrm", 885, 330, "ModR/M · 相对", byte(bytes[1]), { width: 220 })}${register("id-disp", 1120, 330, "disp32 / 有符号", String(loadDisp), { width: 230 })}${pos("id-formula", 650, 470, `后继 RIP = ${loaded.registers.rip}<br>有效地址 = 后继 RIP + ${loadDisp}`, "cycle-formula", { width: 700 })}${register("id-ea", 650, 565, "inputs[0] / 实际有效地址", "—", { width: 350 })}${memory("id-data", 1030, 560, [{ label: "inputs[0] / 读源", address: load.inputs.address, value: load.inputs.values[0], role: "source-row" }], { width: 300, title: "数据内存", direction: "VA ↑ · 原始输入保留", cellHeight: 85 })}${register("id-eax", 650, 665, "%eax / MOV 前 → 后", load.registers.eax, { width: 350 })}${note("id-note", 90, 660, "字节保留，继续解释它。", "字段与有效地址可核对；不是CPU内部译码电路的录屏。", { width: 500 })}${wires(
        "id-wires",
        [
          ["id-ea-flow", "M1370 470V608H1015", "cyan", "地址"],
          ["id-load-flow", "M1015 656V710", "cyan", "03"],
        ],
      )}</div>`,
      14,
      {
        stops: [10.5, 12.5],
        checks: [
          { at: 11, selector: "#id-ea .reg-value", text: loadEA },
          {
            at: 11,
            selector: "#id-eax .reg-value",
            text: loaded.registers.eax,
          },
        ],
      },
    ),
  );
  show(s, "#id-head", 0.1);
  showMemory(s, "id-code", 0.6);
  show(s, "#id-rip", 0.6);
  show(s, "#id-opcode", 0.6);
  show(s, "#id-modrm", 1.4);
  show(s, "#id-disp", 2.2);
  show(s, "#id-formula", 3.1);
  show(s, "#id-ea", 3.8);
  flow(s, "id-ea-flow", 4, 1.4);
  set(s, "#id-ea .reg-value", loadEA, 5.5);
  showMemory(s, "id-data", 5.8);
  show(s, "#id-eax", 6);
  flow(s, "id-load-flow", 7, 1.5);
  set(s, "#id-eax .reg-value", loaded.registers.eax, 8.6);
  set(s, "#id-rip .reg-value", loaded.registers.rip, 8.6);
  show(s, "#id-note", 10);
  return closeShot(s);
}
function execute() {
  const s = make(
    shot(
      "cycle-execute",
      `<div class="camera">${heading("ie-head", 80, 100, "EXECUTE / ACTUAL ADD BEFORE & AFTER", "跳到 ADD 前：两个操作数已经就位。")}${code("ie-asm", 80, 230, [asm(add), byteString(add.instruction.bytes)], { width: 670, title: "GDB / 实际 ADD 与机器码；中间几条指令在实验台" })}${register("ie-eax", 80, 425, "%eax / 执行前", add.registers.eax, { width: 260 })}${register("ie-edx", 400, 425, "%edx / 执行前", add.registers.edx, { width: 260 })}${chip("ie-alu", 790, 330, { label: "ALU", subtitle: "2 + 3 / 机制表达", width: 220 })}${register("ie-result", 1060, 365, "%eax / 同一寄存器写回", "—", { width: 285 })}${register("ie-rip", 80, 605, "%rip / ADD 前 → 后", add.registers.rip, { width: 630 })}${note("ie-note", 790, 620, "ADD 后 EAX=5；不是给一个新寄存器取名EAX", "左侧保留的是执行前读值对照。前面读输入到这里，中间确有其他真实指令。", { width: 555 })}${wires(
        "ie-wires",
        [
          ["ie-left", "M360 500V560H725V385H775", "cyan", "02"],
          ["ie-right", "M680 500C735 500 730 440 775 440", "cyan", "03"],
          ["ie-write", "M1025 425H1045", "cyan", "05"],
        ],
      )}</div>`,
      16,
      {
        stops: [11.5, 14.5],
        checks: [
          {
            at: 12,
            selector: "#ie-result .reg-value",
            text: added.registers.eax,
          },
          { at: 12, selector: "#ie-rip .reg-value", text: added.registers.rip },
        ],
      },
    ),
  );
  show(s, "#ie-head", 0.1);
  show(s, "#ie-asm", 0.6);
  show(s, "#ie-eax", 1);
  show(s, "#ie-edx", 1.3);
  show(s, "#ie-alu", 1.6);
  show(s, "#ie-result", 1.9);
  show(s, "#ie-rip", 2);
  flow(s, "ie-left", 2.5, 1.6);
  flow(s, "ie-right", 3, 1.6);
  flow(s, "ie-write", 5.3, 1.2);
  set(s, "#ie-result .reg-value", added.registers.eax, 6.6);
  set(s, "#ie-rip .reg-value", added.registers.rip, 6.6);
  show(s, "#ie-note", 9.5);
  return closeShot(s);
}
function writeback() {
  const rows = store.output.bytes
    .map((v, i) => ({
      key: String(i),
      label: `observed byte[${i}]`,
      address: hx(BigInt(store.output.address) + BigInt(i)),
      value: byte(v),
      role: "target-row",
    }))
    .reverse();
  const routes = stored.output.bytes.map((v, i) => [
    `wb-${i}`,
    `M705 320C805 320 800 ${335 + (3 - i) * 75} 885 ${335 + (3 - i) * 75}`,
    "cyan",
    byte(v),
  ]);
  const s = make(
    shot(
      "cycle-writeback",
      `<div class="camera">${heading("iw-head", 80, 100, "STORE / ACTUAL MEMORY BEFORE & AFTER", "结果写回内存，输入仍保持原样。")}${register("iw-source", 80, 245, "%eax / store 的真实数据源", store.registers.eax, { width: 600 })}${code("iw-asm", 80, 395, [asm(store)], { width: 690, title: "实际 MOV / EAX → RIP-relative 内存" })}${pos("iw-address", 80, 515, `目标 EA = ${storeEA}`, "cycle-formula", { width: 690 })}${memory("iw-input", 80, 575, inputRows(), { width: 620, title: "原始输入 / 只读源", direction: "VA ↑ · 本次两项始终不变", cellHeight: 56 })}${memory("iw-output", 900, 245, rows, { width: 440, title: "observed / 固定4字节对象", direction: "VA ↑ · 小端字节 · 不是容量增长", cellHeight: 75 })}${note("iw-note", 800, 650, "实际采样：observed 由0变5", "四字节作为一组，在store后快照统一提交；飞行只解释数据去向，不是实测逐字节总线时序。", { width: 545 })}${wires("iw-wires", routes)}</div>`,
      16,
      {
        stops: [11.5, 14.5],
        checks: stored.output.bytes
          .map((v, i) => ({
            at: 12,
            selector: `#iw-output-${i} .array-value`,
            text: byte(v),
          }))
          .concat([
            { at: 12, selector: "#iw-input-0 .array-value", text: "3" },
          ]),
      },
    ),
  );
  show(s, "#iw-head", 0.1);
  show(s, "#iw-source", 0.6);
  show(s, "#iw-asm", 1);
  show(s, "#iw-address", 1.5);
  showMemory(s, "iw-input", 2);
  showMemory(s, "iw-output", 2.3);
  routes.forEach(([key], i) => flow(s, key, 3 + i * 1.25, 0.85));
  stored.output.bytes.forEach((v, i) =>
    set(s, `#iw-output-${i} .array-value`, byte(v), 8),
  );
  show(s, "#iw-note", 9.5);
  return closeShot(s);
}
function nextInstruction() {
  const s = make(
    shot(
      "cycle-next",
      `<div class="camera">${heading("in-head", 80, 100, "NEXT RIP / SEQUENTIAL VS RET", "下一条位置，不总是 RIP 加1。")}${register("in-before", 80, 245, "回看先前 MOV / 原 RIP", load.registers.rip, { width: 650 })}${register("in-sequential", 80, 420, `顺序执行 / +${load.instruction.length}B → 后继RIP`, "—", { width: 650 })}${register("in-value", 80, 605, "%rax / 已完成计算的整数返回值", returned.registers.rax, { width: 650 })}${memory("in-stack", 870, 230, [{ label: "保存的返回地址 / 8B", address: ret.registers.rsp, value: t.returnAddress }], { width: 480, title: "RET 前的栈顶 / 本次保存地址", direction: "VA ↑ · 返回地址不是整数返回值", cellHeight: 100 })}${register("in-ret", 870, 460, "%rip / RET 后", "—", { width: 480 })}${register("in-rsp", 870, 610, "%rsp / RET 前 → 后", ret.registers.rsp, { width: 480 })}${wires(
        "in-wires",
        [
          ["in-seq", "M745 310C795 310 795 480 745 480", "cyan", "+6"],
          ["in-ret-flow", "M855 335C805 335 800 520 855 520", "blue", "RA"],
        ],
      )}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [
          {
            at: 11,
            selector: "#in-sequential .reg-value",
            text: loaded.registers.rip,
          },
          {
            at: 11,
            selector: "#in-ret .reg-value",
            text: returned.registers.rip,
          },
          {
            at: 11,
            selector: "#in-rsp .reg-value",
            text: returned.registers.rsp,
          },
        ],
      },
    ),
  );
  show(s, "#in-head", 0.1);
  show(s, "#in-before", 0.6);
  show(s, "#in-sequential", 1);
  flow(s, "in-seq", 1.5, 1.4);
  set(s, "#in-sequential .reg-value", loaded.registers.rip, 3);
  showMemory(s, "in-stack", 3.8);
  show(s, "#in-ret", 4.2);
  show(s, "#in-rsp", 4.5);
  flow(s, "in-ret-flow", 6, 2);
  set(s, "#in-ret .reg-value", returned.registers.rip, 8.1);
  set(s, "#in-rsp .reg-value", returned.registers.rsp, 8.1);
  show(s, "#in-value", 8.7);
  return closeShot(s);
}
function summary() {
  const roles = [
    ["取指", "RIP 找到指令字节"],
    ["译码", "确定操作与寻址"],
    ["执行", "读操作数，完成运算"],
    ["写回／继续", "提交结果，选后继"],
  ];
  const s = make(
    shot(
      "cycle-summary",
      `<div class="camera">${heading("is-head", 80, 100, "ARCHITECTURAL STATES + EXPLANATORY MODEL", `${cycleSummary.samples}个真实停点，支撑一条计算路径。`)}${pos("is-roles", 100, 260, roles.map(([title, detail], i) => `<div id="is-role-${i}" class="cycle-role" data-anim><strong>${title}</strong>${detail}</div>`).join(""), "cycle-roles")}${register("is-observed", 100, 450, "内存 observed", String(stored.output.value), { width: 340 })}${register("is-rax", 550, 450, "%rax / 函数返回", returned.registers.rax, { width: 340 })}${register("is-exit", 1000, 450, "程序实际退出码", String(t.exitCode), { width: 340 })}${note("is-note", 100, 635, "这不是微架构时钟、流水线或推测执行的录屏", "指令与前后值可核对；内部动作、箭头与教学速度是解释设计。下一计划项为第2章字节序。", { width: 1210 })}</div>`,
      10,
      {
        stops: [7.5, 9],
        checks: [
          { at: 8, selector: "#is-observed .reg-value", text: "5" },
          { at: 8, selector: "#is-exit .reg-value", text: "0" },
        ],
      },
    ),
  );
  show(s, "#is-head", 0.1);
  show(s, "#is-roles", 0.5);
  roles.forEach((_, i) => show(s, "#is-role-" + i, 0.8 + i * 0.6));
  show(s, "#is-observed", 3.5);
  show(s, "#is-rax", 4);
  show(s, "#is-exit", 4.5);
  show(s, "#is-note", 5.7);
  return closeShot(s);
}
const p = t.provenance;
const api = mountFilm({
  id: "instruction-cycle",
  chapter: 1,
  title: "一条机器指令如何执行？取指、译码与执行",
  model:
    "C／GDB 逐指令架构状态实测 + 内部机制解释 · 非真实硬件时钟 · 内存高地址在上",
  example: "../experiments/instruction-cycle.c",
  provenance: {
    ...p,
    samples: cycleSummary.samples,
    events: cycleSummary.events,
    sourceHref: "../experiments/instruction-cycle.c",
    jsonHref: "../traces/instruction-cycle.json",
    gdbHref: "../traces/instruction-cycle-gdb.txt",
  },
  experiment,
  actorContinuity: [
    actor('mov-code-bytes','array',[['cycle-fetch','#if-code'],['cycle-decode','#id-code']]),
    actor('mov-rip','register',[['cycle-fetch','#if-rip'],['cycle-decode','#id-rip']]),
    actor('computed-rax','register',[['cycle-execute','#ie-result'],['cycle-writeback','#iw-source'],['cycle-next','#in-value'],['cycle-summary','#is-rax']]),
  ],
  scenes: [
    { title: "从完整 C 实验出发", view: "C / ACTUAL DATA", build: intro },
    {
      title: "RIP 指向字节：取指",
      view: "FETCH MODEL / REAL BYTES",
      build: fetchInstruction,
    },
    { title: "译码与有效地址", view: "DECODE MODEL / REAL EA", build: decode },
    { title: "ADD 与寄存器写回", view: "ACTUAL ADD STATES", build: execute },
    { title: "STORE 提交内存", view: "ACTUAL STORE STATES", build: writeback },
    {
      title: "顺序后继与 RET",
      view: "ACTUAL RIP TRANSITIONS",
      build: nextInstruction,
    },
    {
      title: "一条计算路径的闭环",
      view: "EVIDENCE / MODEL BOUNDARY",
      build: summary,
    },
  ],
});
const links = document.querySelector(".experiment-links");
links.insertAdjacentHTML(
  "beforeend",
  '<a href="../traces/instruction-cycle/program.elf">真实实验 ELF ↗</a><a href="../traces/instruction-cycle/disassembly.txt">完整反汇编 ↗</a><a href="./instruction-cycle.md">本片实验说明 ↗</a>',
);
const effects = document.createElement("pre");
effects.id = "cycle-step-effects";
effects.className = "cycle-effects";
document.querySelector("#experiment-command-output").after(effects);
function updateEffects() {
  const i = Number(document.querySelector("#experiment-stop").value),
    before = t.samples[i],
    after = t.samples[i + 1];
  effects.textContent = after
    ? `实际下一次单步的前后状态（不是CPU内部时钟）\nRIP ${before.registers.rip} → ${after.registers.rip}\nEAX ${before.registers.eax} → ${after.registers.eax}\nobserved ${before.output.value} → ${after.output.value}`
    : "已返回main；本次指令采集在此停止，随后实际继续执行到退出0。";
}
document
  .querySelector("#experiment-stop")
  .addEventListener("change", updateEffects);
updateEffects();
