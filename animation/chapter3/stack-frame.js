import { mountFilm } from "../shared/player.js";
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
  frame,
  chip,
  windowBox,
  wires,
  flow,
  reveal,
  state,
  camera,
} from "../shared/scene-kit.js";
import {
  trace,
  measured,
  reg,
  local,
  decimal,
  allocation,
  saved,
  sourceLines,
  sourceTitle,
  sourceLineNumbers,
  prologue,
  epilogue,
} from "../models/stack-trace.js";

// No measured addresses are hard-coded here. All numeric state comes from one GDB capture.
const RA_MAIN = saved("caller", "return").value;
const RA_CALLER = saved("add", "return").value;
const ARG = decimal(reg("main_before_call", "rdi"));
const A = local("add_frame_ready", "add", "a").value;
const B = local("add_frame_ready", "add", "b").value;
const RESULT = local("add_result_ready", "add", "sum").value;
const addressRegister = (...args) =>
  register(...args).replace("register-cell", "register-cell measured-address");
const checkReg = (at, selector, event, key) => ({
  at,
  selector: `${selector} .reg-value`,
  text: reg(event, key),
});
const setReg = (s, selector, event, key, at) =>
  number(s.timeline, s.$(`${selector} .reg-value`), reg(event, key), at);

function parameters() {
  const mainCall = sourceLines("main")
    .find((line) => line.includes("caller("))
    .trim();
  const rows = [...sourceLines("caller"), mainCall];
  const s = shot(
    "call-parameters",
    `<div class="camera">
    ${pos("call-origin", 592, 285, `<div class="function-code">caller<span class="parens">(${ARG})</span></div><div class="eyebrow" style="margin-top:18px">GDB / REAL EXECUTION</div>`, "", { visible: true })}
    ${code("call-source", 180, 305, rows, { width: 650, title: "stack-call.c / 真实源码节选", lineNumbers: [...sourceLineNumbers("caller"), measured.main_before_call.source.line] })}
    ${register("parameter-rdi", 1000, 330, "%rdi · 第一个整数参数", "—", { width: 280 })}
    ${note("parameter-note", 1000, 500, "在 call 前实际采集", `rdi = ${reg("main_before_call", "rdi")}，不是入口处猜测形参。`, { width: 300 })}
    ${wires("parameter-wires", [["parameter-flight", "M740 455C850 455 820 385 982 385", "cyan", String(ARG)]])}</div>`,
    7,
    {
      stops: [4.9, 6.2],
      checks: [
        { at: 5.5, selector: "#parameter-rdi .reg-value", text: String(ARG) },
      ],
    },
  );
  s.timeline.to(
    s.$("#call-origin"),
    {
      x: -410,
      y: -155,
      scale: 0.68,
      transformOrigin: "0 0",
      duration: 1.1,
      ease: "power3.inOut",
    },
    0.3,
  );
  reveal(s.timeline, s.$("#call-source"), 1.2, { x: -20, y: 0 });
  activeCode(s, rows.length - 1, 1.8);
  reveal(s.timeline, s.$("#parameter-rdi"), 2.2, { y: 0, scale: 0.96 });
  flow(s, "parameter-flight", 2.7, 1.2);
  number(s.timeline, s.$("#parameter-rdi .reg-value"), ARG, 4);
  reveal(s.timeline, s.$("#parameter-note"), 4.2);
  state(s, "parameters-ready", 4);
  return closeShot(s);
}
function enterCaller() {
  const variable = local("caller_frame_ready", "caller", "local");
  const slots = [
    ["返回 main", RA_MAIN],
    ["保存旧 %rbp", saved("caller", "rbp").value],
    ["实际局部预留", `${allocation("caller")} bytes`],
    [`local @ ${variable.address}`, String(variable.value)],
  ]
    .map(([label, value], i) =>
      pos(
        `caller-slot-${i}`,
        850,
        240 + i * 52,
        `<span>${label}</span><code>${value}</code>`,
        `stack-slot ${i === 0 ? "return-slot" : ""}`,
      ),
    )
    .join("");
  const s = shot(
    "call-enter-caller",
    `<div class="camera">${chip("caller-cpu", 340, 290)}${addressRegister("caller-rsp", 170, 155, "%rsp", reg("main_before_call", "rsp"), { width: 280 })}${addressRegister("caller-rbp", 480, 155, "%rbp", reg("main_before_call", "rbp"), { width: 280 })}${slots}
    ${heading("caller-heading", 850, 120, "CALL / MEASURED PROLOGUE", "留下<em>真实返回位置</em>。")}
    ${note("caller-note", 850, 510, `本次实际分配 ${allocation("caller")}B`, "不假设每个编译结果都分配 16B；布局不按比例。")}
    ${wires("caller-wires", [["caller-ret", "M580 370C720 370 700 263 832 263", "blue", "RA"]])}
    ${code("caller-instructions", 160, 590, prologue("caller"), { width: 600, title: "GDB 反汇编 / 当前 ELF" })}</div>`,
    8,
    {
      stops: [2.8, 5.6, 7],
      checks: [
        checkReg(3, "#caller-rsp", "caller_entry", "rsp"),
        checkReg(6, "#caller-rsp", "caller_frame_ready", "rsp"),
        checkReg(6, "#caller-rbp", "caller_frame_ready", "rbp"),
        { at: 6, selector: "#caller-slot-0 code", text: RA_MAIN },
      ],
    },
  );
  ["#caller-cpu", "#caller-rsp", "#caller-rbp"].forEach((e, i) =>
    reveal(s.timeline, s.$(e), 0.1 + i * 0.12),
  );
  reveal(s.timeline, s.$("#caller-heading"), 0.6);
  flow(s, "caller-ret", 0.9, 1.2);
  reveal(s.timeline, s.$("#caller-slot-0"), 2.1, { x: 20, y: 0 });
  setReg(s, "#caller-rsp", "caller_entry", "rsp", 2.3);
  for (let i = 1; i < 4; i++)
    reveal(s.timeline, s.$(`#caller-slot-${i}`), 3 + i * 0.45, { y: -18 });
  setReg(s, "#caller-rsp", "caller_frame_ready", "rsp", 5);
  setReg(s, "#caller-rbp", "caller_frame_ready", "rbp", 5);
  reveal(s.timeline, s.$("#caller-note"), 5.3);
  reveal(s.timeline, s.$("#caller-instructions"), 1.5);
  activeCode(s, 0, 1.9);
  activeCode(s, prologue("caller").length - 1, 4.8);
  state(s, "caller-frame-ready", 5);
  return closeShot(s);
}
function nested() {
  const callLine = sourceLines("caller")
    .find((line) => line.includes("add("))
    .trim();
  const s = shot(
    "call-nested",
    `<div class="camera">${frame("nested-caller", `caller(${ARG})`, { x: 850, y: 150, content: "返回 main", value: RA_MAIN, animated: false })}
    ${frame("nested-add", `add(${A}, ${B})`, { x: 850, y: 374, index: "02", content: "返回 caller", value: RA_CALLER })}
    ${register("nested-rdi", 180, 210, "%rdi", String(A))}${register("nested-rsi", 470, 210, "%rsi", "—")}
    ${addressRegister("nested-rsp", 180, 485, "%rsp", reg("caller_frame_ready", "rsp"), { width: 280 })}${addressRegister("nested-rbp", 480, 485, "%rbp", reg("caller_frame_ready", "rbp"), { width: 280 })}
    ${pos("nested-wait", 850, 292, "caller 等待 add 返回", "soft-label")}${code("nested-code", 180, 370, [callLine], { width: 580, title: sourceTitle("caller"), lineNumbers: [measured.caller_before_add.source.line] })}
    ${note("nested-note", 850, 548, `add 实测局部预留 ${allocation("add")}B`, "参数值来自 call 前寄存器；序言后才读取局部变量。")}
    ${wires("nested-wires", [
      [
        "nested-argument",
        "M775 435H795V270H725",
        "cyan",
        String(B),
      ],
      ["nested-frame", "M1185 202C1290 202 1290 425 1185 425", "blue"],
    ])}</div>`,
    8,
    {
      stops: [2.8, 4.5, 6.6],
      checks: [
        checkReg(4.2, "#nested-rsp", "add_entry", "rsp"),
        checkReg(6.8, "#nested-rsp", "add_frame_ready", "rsp"),
        checkReg(6.8, "#nested-rbp", "add_frame_ready", "rbp"),
        {
          at: 6.8,
          selector: "#nested-add .frame-content code",
          text: RA_CALLER,
        },
      ],
    },
  );
  ["#nested-rsp", "#nested-rbp"].forEach(e=>reveal(s.timeline,s.$(e),0.2));
  ["#nested-rdi", "#nested-rsi", "#nested-code"].forEach(e=>reveal(s.timeline,s.$(e),0.72,{y:0,duration:0.2}));
  flow(s, "nested-argument", 1, 1);
  number(
    s.timeline,
    s.$("#nested-rsi .reg-value"),
    decimal(reg("caller_before_add", "rsi")),
    2.1,
  );
  flow(s, "nested-frame", 2.6, 1.1);
  reveal(s.timeline, s.$("#nested-add"), 3.5, { y: -25 });
  setReg(s, "#nested-rsp", "add_entry", "rsp", 3.8);
  reveal(s.timeline, s.$("#nested-wait"), 3.8);
  s.timeline.to(s.$("#nested-caller"), { opacity: 0.5, duration: 0.5 }, 3.7);
  setReg(s, "#nested-rsp", "add_frame_ready", "rsp", 5.2);
  setReg(s, "#nested-rbp", "add_frame_ready", "rbp", 5.2);
  s.timeline.to(
    s.$("#nested-add .frame-active"),
    { opacity: 1, duration: 0.5 },
    4.2,
  );
  reveal(s.timeline, s.$("#nested-note"), 5.5);
  state(s, "nested-frame-ready", 5.2);
  return closeShot(s);
}
function calculation() {
  const sum = local("add_result_ready", "add", "sum");
  const s = shot(
    "call-calculation",
    `<div class="camera">${heading("sum-heading", 170, 135, "SOURCE + REGISTER / CAPTURED", "算出结果，<em>不是地址</em>。")}
    ${addressRegister("sum-rsp",80,255,"%rsp",reg("add_result_ready","rsp"),{width:280})}${addressRegister("sum-rbp",1040,145,"%rbp",reg("add_result_ready","rbp"),{width:280})}${pos("sum-a", 390, 336, String(A), "inline-value")}${pos("sum-plus", 575, 338, "+", "inline-value")}${pos("sum-b", 730, 336, String(B), "inline-value")}${pos("sum-five", 595, 336, String(RESULT), "inline-value")}
    ${register("sum-rax", 1040, 310, "%rax · 返回值", "—", { width: 240 })}
    ${code("sum-source", 170, 450, sourceLines("add"), { width: 600, title: sourceTitle("add"), lineNumbers: sourceLineNumbers("add") })}
    ${note("sum-note", 850, 555, `sum @ ${sum.address}`, `GDB 在 return sum 前采到 sum=${sum.value}；rax=${reg("add_result_ready", "rax")}。`, { width: 440 })}
    ${wires("sum-wires", [["sum-result", "M650 370C810 370 856 365 1022 365", "cyan", String(RESULT)]])}</div>`,
    7,
    {
      stops: [3.7, 5.7],
      checks: [
        { at: 5.8, selector: "#sum-rax .reg-value", text: String(RESULT) },
      ],
    },
  );
  reveal(s.timeline, s.$("#sum-heading"), 0.1);
  reveal(s.timeline, s.$("#sum-rsp"), 0.1);
  reveal(s.timeline, s.$("#sum-rbp"), 0.1);
  ["#sum-a", "#sum-plus", "#sum-b"].forEach((e, i) =>
    reveal(s.timeline, s.$(e), 0.6 + i * 0.4),
  );
  s.timeline.to(s.$("#sum-a"), { x: 155, opacity: 0, duration: 0.8 }, 2);
  s.timeline.to(s.$("#sum-b"), { x: -135, opacity: 0, duration: 0.8 }, 2);
  s.timeline.to(s.$("#sum-plus"), { autoAlpha: 0, duration: 0.3 }, 2.3);
  reveal(s.timeline, s.$("#sum-five"), 2.8, { y: 0, scale: 0.7 });
  reveal(s.timeline, s.$("#sum-rax"), 3);
  flow(s, "sum-result", 3.4, 1.1);
  number(
    s.timeline,
    s.$("#sum-rax .reg-value"),
    decimal(reg("add_result_ready", "rax")),
    4.6,
  );
  reveal(s.timeline, s.$("#sum-source"), 4);
  activeCode(s, 3, 4.8);
  reveal(s.timeline, s.$("#sum-note"), 5);
  state(s, "rax-holds-five", 4.6);
  return closeShot(s);
}
function returnCaller() {
  const s = shot(
    "call-return-caller",
    `<div class="camera">${frame("return-caller-frame", "caller · 等待", { x: 850, y: 145, content: "local", value: String(local("caller_frame_ready", "caller", "local").value), animated: true })}${frame("return-add-frame", "add · 即将返回", { x: 850, y: 330, index: "02", value: RA_CALLER, animated: true })}
    ${addressRegister("return-add-rsp", 170, 145, "%rsp", reg("add_result_ready", "rsp"), { width: 280 })}${addressRegister("return-add-rbp", 480, 145, "%rbp", reg("add_result_ready", "rbp"), { width: 280 })}
    ${addressRegister("return-add-rip", 170, 430, "%rip", reg("add_result_ready", "rip"), { width: 280 })}${register("return-add-rax", 480, 430, "%rax", String(RESULT))}
    ${note("return-add-note", 850, 520, "ret 的下一帧采样确实回到 caller", "实际栈顶值等于 call 后那条指令的地址。")}
    ${code("return-add-code", 170, 620, epilogue("add"), { width: 600, title: "GDB 反汇编 / 实际尾声" })}
    ${wires("return-add-wires", [["return-add-control", "M832 385H450V490", "blue", "RA"]])}</div>`,
    10,
    {
      stops: [3.5, 6.9, 8.5],
      checks: [
        checkReg(3.8, "#return-add-rsp", "add_ret", "rsp"),
        checkReg(3.8, "#return-add-rip", "add_ret", "rip"),
        checkReg(7.2, "#return-add-rsp", "caller_after_add", "rsp"),
        checkReg(7.2, "#return-add-rip", "caller_after_add", "rip"),
        { at: 8, selector: "#return-add-rax .reg-value", text: String(RESULT) },
      ],
    },
  );
  [
    "#return-add-rsp",
    "#return-add-rbp",
    "#return-add-rip",
    "#return-add-rax",
  ].forEach((e) => reveal(s.timeline, s.$(e), 0.1));
  reveal(s.timeline, s.$("#return-caller-frame"), 1.2, {y:0});
  reveal(s.timeline, s.$("#return-add-frame"), 1.2, {y:0});
  reveal(s.timeline, s.$("#return-add-code"), 0.7);
  activeCode(s, 0, 1);
  setReg(s, "#return-add-rsp", "add_ret", "rsp", 3);
  setReg(s, "#return-add-rbp", "add_ret", "rbp", 3);
  setReg(s, "#return-add-rip", "add_ret", "rip", 3);
  s.timeline.to(
    s.$("#return-add-frame"),
    { height: 68, y: -7, duration: 1 },
    2.1,
  );
  activeCode(s, 1, 4.2);
  flow(s, "return-add-control", 4.5, 1.3);
  setReg(s, "#return-add-rip", "caller_after_add", "rip", 5.9);
  setReg(s, "#return-add-rsp", "caller_after_add", "rsp", 5.9);
  s.timeline.to(
    s.$("#return-add-frame"),
    { opacity: 0.18, duration: 0.7 },
    5.9,
  );
  reveal(s.timeline, s.$("#return-add-note"), 6.3);
  state(s, "returned-to-caller", 5.9);
  camera(s, 7.8, { y: -18, duration: 1 });
  return closeShot(s);
}
function returnMain() {
  const value = local("main_result_stored", "main", "result");
  const mainCall = sourceLines("main")
    .find((line) => line.includes("caller("))
    .trim();
  const s = shot(
    "call-return-main",
    `<div class="camera">${windowBox("return-main-window", 170, 185, "main() / 实际源代码", `<code>${mainCall}</code><small>正在等待返回</small>`, { width: 575, visible: false })}
    ${frame("return-main-frame", "caller · 返回 main", { x: 850, y: 200, value: RA_MAIN, animated: true })}
    ${addressRegister("return-main-rsp", 850, 430, "%rsp", reg("caller_after_add", "rsp"), { width: 260 })}${addressRegister("return-main-rbp", 1130, 430, "%rbp", reg("caller_after_add", "rbp"), { width: 260 })}
    ${register("return-main-rax", 170, 475, "%rax", String(RESULT))}${pos("return-main-result", 472, 480, String(value.value), "inline-value")}
    ${note("return-main-note", 170, 620, `result @ ${value.address}`, `main 真实写回 ${value.value}；rsp/rbp 恢复到 call 前。`, { width: 800 })}
    ${wires("return-main-wires", [
      ["return-main-control", "M835 255C790 255 800 265 763 265", "blue", "RA"],
      [
        "return-main-value",
        "M420 528C460 528 455 515 480 515",
        "cyan",
        String(RESULT),
      ],
    ])}</div>`,
    8,
    {
      stops: [2.8, 5.2, 7],
      checks: [
        checkReg(2.1, "#return-main-rsp", "caller_ret", "rsp"),
        checkReg(6.5, "#return-main-rsp", "main_after_call", "rsp"),
        checkReg(6.5, "#return-main-rbp", "main_after_call", "rbp"),
        {
          at: 6.5,
          selector: "#return-main-window small",
          text: `result = ${value.value} · main 继续运行`,
        },
      ],
    },
  );
  ["#return-main-rsp", "#return-main-rbp", "#return-main-rax"].forEach((e) =>
    reveal(s.timeline, s.$(e), 0.2),
  );
  reveal(s.timeline, s.$("#return-main-window"), 1.2, {y:0});
  reveal(s.timeline, s.$("#return-main-frame"), 1.2, {y:0});
  setReg(s, "#return-main-rsp", "caller_ret", "rsp", 1.8);
  setReg(s, "#return-main-rbp", "caller_ret", "rbp", 1.8);
  flow(s, "return-main-control", 2.4, 1);
  setReg(s, "#return-main-rsp", "main_after_call", "rsp", 3.5);
  s.timeline.to(
    s.$("#return-main-frame"),
    { opacity: 0.18, duration: 0.7 },
    3.5,
  );
  flow(s, "return-main-value", 4.1, 0.8);
  reveal(s.timeline, s.$("#return-main-result"), 4.9, { y: 0 });
  number(
    s.timeline,
    s.$("#return-main-window small"),
    `result = ${value.value} · main 继续运行`,
    5,
  );
  reveal(s.timeline, s.$("#return-main-note"), 5.3);
  state(s, "main-received-five", 5);
  return closeShot(s, { exit: false });
}
mountFilm({
  id: "stack-frame",
  title: "函数调用与栈帧 · GDB 实测",
  chapter: 3,
  actorContinuity: [
    actor('live-rsp','register',[['call-enter-caller','#caller-rsp'],['call-nested','#nested-rsp',{move:0.65}],['call-calculation','#sum-rsp'],['call-return-caller','#return-add-rsp'],['call-return-main','#return-main-rsp']]),
    actor('live-rbp','register',[['call-enter-caller','#caller-rbp'],['call-nested','#nested-rbp',{move:0.65}],['call-calculation','#sum-rbp'],['call-return-caller','#return-add-rbp'],['call-return-main','#return-main-rbp']]),
    actor('return-value-rax','register',[['call-calculation','#sum-rax'],['call-return-caller','#return-add-rax'],['call-return-main','#return-main-rax']]),
  ],
  model:
    "GDB 单次实测回放 · PIE / ASLR 保持 · 数值来自采样，图形布局不按字节比例",
  example: "../experiments/stack-call.c",
  provenance: {
    ...trace.provenance,
    architecture: trace.architecture,
    samples: trace.samples.length,
    events: Object.keys(trace.events).length,
    sourceHref: "../experiments/stack-call.c",
    jsonHref: "../traces/stack-call.json",
    gdbHref: "../traces/stack-call-gdb.txt",
  },
  scenes: [
    { title: "真实调用", detail: "源码与参数寄存器", build: parameters },
    { title: "实际栈帧", detail: "采样决定分配量", build: enterCaller },
    { title: "嵌套调用", detail: "两份独立的帧", build: nested },
    { title: "结果就绪", detail: "变量与 rax 核对", build: calculation },
    { title: "返回 caller", detail: "ret 前后逐指令核对", build: returnCaller },
    { title: "恢复 main", detail: "实际写回 result", build: returnMain },
  ],
});
