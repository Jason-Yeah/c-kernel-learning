import { actor } from "../shared/actor-continuity.js";
import {
  shot,
  closeShot,
  heading,
  note,
  pos,
  register,
  code,
  chip,
  wires,
  flow,
  reveal,
} from "../shared/scene-kit.js";
import {
  memory,
  showMemory,
  actualCode,
  commit,
  osShot,
  mountOS,
} from "../shared/os-components.js";
import {
  addEntry,
  addReturned,
  widthBefore,
  widthAfter,
  compared,
  flagBits,
  computed,
  hexAddress,
  sourceRows,
} from "../models/os-basics.js";
function bank() {
  const groups = [
    [
      "整数参数",
      "RDI RSI<br>RDX RCX<br>R8 R9",
      "前6个常见整数／指针参数。<br>这是 Linux SysV 函数 ABI。",
    ],
    [
      "被调用者保存",
      "RBX RBP<br>R12 R13<br>R14 R15",
      "若函数使用这些寄存器，<br>返回前应恢复原值。",
    ],
    [
      "结果与临时值",
      "RAX<br>R10 R11",
      "RAX 常用作整数返回值；<br>调用者不能假设临时值保留。",
    ],
    [
      "栈与控制",
      "RSP<br>RIP<br>RFLAGS",
      "RSP 是栈顶指针；<br>RIP 是当前指令，标志描述状态。",
    ],
  ];
  const s = osShot(
    shot(
      "regs-bank",
      `<div class="camera">${heading("rb-head", 80, 100, "X86-64 / INTEGER REGISTERS + CONTROL", "寄存器，是 CPU 当前正在使用的现场。")}${pos("rb-bank", 100, 270, groups.map((g, i) => `<div id="rb-group-${i}" class="os-reg-group" data-anim><div class="os-role">${g[0]}</div><strong>${g[1]}</strong><span>${g[2]}</span></div>`).join(""), "os-chip-registers", { width: 1240 })}${note("rb-note", 100, 615, "通用寄存器的作用，既有 ISA 规则，也有 ABI 约定", "RBP 在本构建作帧指针，并非总是如此；x86 还有向量、段和控制等其他寄存器。本片不把这组当成完整寄存器全集。", { width: 1210 })}</div>`,
      16,
      {
        stops: [10.5, 14.5],
        checks: [{ at: 11, selector: "#rb-group-3", shown: true }],
      },
    ),
  );
  reveal(s.timeline, s.$("#rb-head"), 0.1);
  reveal(s.timeline, s.$("#rb-bank"), 0.6);
  groups.forEach((_, i) =>
    reveal(s.timeline, s.$(`#rb-group-${i}`), 1.1 + i * 1.6, { x: 30, y: 0 }),
  );
  reveal(s.timeline, s.$("#rb-note"), 9);
  return closeShot(s);
}
function call() {
  const callerRSP = addReturned.registers.rsp,
    calleeRSP = addEntry.registers.rsp;
  const rows = [
    {
      key: "caller",
      label: "调用者当前栈顶",
      address: callerRSP,
      value: addReturned.stackTop,
    },
    {
      key: "return",
      label: "本次保存的返回地址",
      address: calleeRSP,
      value: "—",
    },
  ];
  const s = osShot(
    shot(
      "regs-call",
      `<div class="camera">${heading("rc-head", 80, 100, "CALL / ACTUAL ENTRY STACK", "call：保存后继位置，再跳到目标。")}${register("rc-rip", 80, 270, "%rip / 控制位置", "—", { width: 560 })}${register("rc-rsp", 80, 435, "%rsp / 当前栈顶", callerRSP, { width: 600 })}${memory("rc-stack", 810, 240, rows, { width: 530, title: "实际入口栈 / 每格8B", direction: "高地址 ↑ · 栈通常向低地址扩展", cellHeight: 98 })}${register("rc-target", 810, 540, "call 的目标 / os_add", addEntry.registers.rip, { width: 530 })}${note("rc-note", 80, 650, "RIP、返回地址和 RAX 返回值，不是一回事", "栈顶保存的是后继指令地址；RBP 是否用作帧指针取决于本次编译。", { width: 1200 })}${wires(
        "rc-wires",
        [
          ["rc-push", "M695 350C750 350 740 440 795 440", "blue", "保存"],
          ["rc-jump", "M795 600C735 600 740 330 695 330", "cyan", "跳转"],
        ],
      )}</div>`,
      18,
      {
        stops: [10.5, 16.5],
        checks: [
          { at: 12, selector: "#rc-rsp .reg-value", text: calleeRSP },
          {
            at: 12,
            selector: "#rc-stack-return .array-value",
            text: addEntry.stackTop,
          },
          {
            at: 12,
            selector: "#rc-rip .reg-value",
            text: addEntry.registers.rip,
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#rc-head"), 0.1);
  reveal(s.timeline, s.$("#rc-rip"), 0.6);
  reveal(s.timeline, s.$("#rc-rsp"), 1);
  showMemory(s, "rc-stack", 1.4);
  flow(s, "rc-push", 2, 1.5);
  commit(s, "#rc-stack-return .array-value", addEntry.stackTop, 3.6);
  commit(s, "#rc-rsp .reg-value", calleeRSP, 3.6);
  reveal(s.timeline, s.$("#rc-target"), 4);
  flow(s, "rc-jump", 5, 2);
  commit(s, "#rc-rip .reg-value", addEntry.registers.rip, 7.1);
  reveal(s.timeline, s.$("#rc-note"), 10);
  return closeShot(s);
}
function argumentsAndReturn() {
  const s = osShot(
    shot(
      "regs-arguments",
      `<div class="camera">${heading("ra-head", 80, 100, "SYSV ABI / OBSERVED ARGUMENTS AND RETURN", "参数进来，结果从 RAX 回去。")}${register("ra-rip",995,210,"%rip / 当前控制位置",addEntry.registers.rip,{width:350})}${register("ra-rsp",675,210,"%rsp / 当前栈顶",addEntry.registers.rsp,{width:280})}${actualCode("ra-source", 80, 240, "os-math.c", ["int os_add(int left, int right)", "return left + right;"], { width: 550 })}${register("ra-rdi", 80, 445, "%rdi / 第1参数", String(Number(BigInt(addEntry.registers.rdi))), { width: 250 })}${register("ra-rsi", 380, 445, "%rsi / 第2参数", String(Number(BigInt(addEntry.registers.rsi))), { width: 250 })}${chip("ra-alu", 675, 310, { label: "ADD", subtitle: "实际 os_add(3, 2)", width: 220 })}${register("ra-rax", 995, 330, "%rax / 整数返回值", "—", { width: 350 })}${register("ra-result", 80, 610, "C / os_result", "—", { width: 550 })}${note("ra-note", 770, 575, "ret 回到 main 的下一条指令", `GDB RIP=${addReturned.registers.rip}；RAX 仍是5。参数顺序是函数 ABI，不是 Windows 或 syscall 的通用规则。`, { width: 580 })}${wires(
        "ra-wires",
        [
          ["ra-left", "M350 520V565H650V390H660", "cyan", "参数"],
          ["ra-right", "M650 520C670 520 650 440 660 440", "cyan", "参数"],
          ["ra-output", "M910 405H980", "cyan", "结果"],
          ["ra-return", "M980 455C935 455 935 675 650 675", "blue", "返回"],
        ],
      )}</div>`,
      16,
      {
        stops: [10.5, 14.5],
        checks: [
          {
            at: 12,
            selector: "#ra-rax .reg-value",
            text: addReturned.registers.rax,
          },
          { at: 12, selector: "#ra-result .reg-value", text: "5" },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#ra-head"), 0.1);
  reveal(s.timeline, s.$("#ra-rip"), 0.3);
  reveal(s.timeline, s.$("#ra-rsp"), 0.3);
  reveal(s.timeline, s.$("#ra-source"), 0.6);
  reveal(s.timeline, s.$("#ra-rdi"), 1);
  reveal(s.timeline, s.$("#ra-rsi"), 1.3);
  flow(s, "ra-left", 2, 1.5);
  flow(s, "ra-right", 2.3, 1.5);
  reveal(s.timeline, s.$("#ra-alu"), 3.9);
  flow(s, "ra-output", 4.7, 1.2);
  reveal(s.timeline, s.$("#ra-rax"), 6);
  commit(s, "#ra-rax .reg-value", addReturned.registers.rax, 6.05);
  reveal(s.timeline, s.$("#ra-result"), 6.3);
  flow(s, "ra-return", 6.6, 2);
  commit(s, "#ra-result .reg-value", computed.result, 8.7);
  commit(s, "#ra-rip .reg-value", addReturned.registers.rip, 8.7);
  commit(s, "#ra-rsp .reg-value", addReturned.registers.rsp, 8.7);
  reveal(s.timeline, s.$("#ra-note"), 10);
  return closeShot(s);
}
function width() {
  const before = widthBefore.registers.rax,
    after = widthAfter.registers.rax;
  const upper = (value) =>
    ((BigInt(value) >> 32n) & 0xffffffffn)
      .toString(16)
      .toUpperCase()
      .padStart(8, "0");
  const lower = (value) =>
    (BigInt(value) & 0xffffffffn).toString(16).toUpperCase().padStart(8, "0");
  const bits = (id, value) =>
    `<div class="os-small">同一寄存器：高32位 / 低32位</div><div class="os-formula" style="border-top:1px solid #fff;margin-top:14px"><span id="${id}-hi">${upper(value)}</span> | <span id="${id}-lo">${lower(value)}</span></div>`;
  const s = osShot(
    shot(
      "regs-width",
      `<div class="camera">${heading("rw-head", 80, 100, "RAX / EAX / AX / AL", "写 EAX，会清零 RAX 的高32位。")}${register("rw-before", 80, 270, "%rax / 实际指令前", before, { width: 600 })}${register("rw-after", 800, 270, "%rax / 实际指令后", "—", { width: 540 })}${pos("rw-old-bits", 80, 440, bits("rw-old", before), "", { width: 600 })}${pos("rw-new-bits", 800, 440, bits("rw-new", before), "", { width: 540 })}${code("rw-instruction", 80, 580, [widthBefore.instruction.asm], { width: 600, title: "GDB / 实际执行的指令" })}${note("rw-note", 800, 600, "64位值，不是两块 RAM", "32位目的寄存器写会清高位；AX／AL 的部分写不具有同样规则。", { width: 540 })}${wires("rw-wires", [["rw-write", "M700 340H785", "cyan", "写"]])}</div>`,
      16,
      {
        stops: [10.5, 14.5],
        checks: [
          { at: 12, selector: "#rw-after .reg-value", text: after },
          { at: 12, selector: "#rw-new-hi", text: upper(after) },
          { at: 12, selector: "#rw-new-lo", text: lower(after) },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#rw-head"), 0.1);
  reveal(s.timeline, s.$("#rw-before"), 0.6);
  reveal(s.timeline, s.$("#rw-old-bits"), 1);
  reveal(s.timeline, s.$("#rw-instruction"), 1.4);
  flow(s, "rw-write", 3, 2);
  reveal(s.timeline, s.$("#rw-after"), 5.1);
  reveal(s.timeline, s.$("#rw-new-bits"), 5.1);
  commit(s, "#rw-after .reg-value", after, 5.2);
  commit(s, "#rw-new-hi", upper(after), 5.2);
  commit(s, "#rw-new-lo", lower(after), 5.2);
  reveal(s.timeline, s.$("#rw-note"), 8.5);
  return closeShot(s);
}
function flags() {
  const labels = {
    ZF: "结果为零",
    CF: "进位／借位",
    SF: "结果符号位",
    OF: "有符号溢出",
  };
  const keys = ["ZF", "CF", "SF", "OF"];
  const regs = keys
    .map((key, i) =>
      register(
        "rf-" + key,
        850 + (i % 2) * 270,
        290 + Math.floor(i / 2) * 150,
        key + " / " + labels[key],
        "—",
        { width: 250 },
      ),
    )
    .join("");
  const s = osShot(
    shot(
      "regs-flags",
      `<div class="camera">${heading("rf-head", 80, 100, "CMP / ACTUAL RFLAGS SNAPSHOT", "比较不只是产生一个整数结果。")}${actualCode("rf-c", 80, 240, "os-math.c", ["int os_equal(int left, int right)", "return left == right;"], { width: 560 })}${code("rf-cmp", 80, 435, [compared.previousInstruction.asm], { width: 560, title: "GDB / 本次 cmp 的真实指令" })}${register("rf-rflags", 80, 575, "%rflags / GDB eflags 值", compared.registers.eflags, { width: 600 })}${regs}${note("rf-note", 850, 630, "本次比较 3 与 3，ZF=1", "只解码这些状态位；不把调试单步的 TF 当作业务条件。C 最终结果为1。", { width: 510 })}${wires("rf-wires", [["rf-compare", "M660 480C750 480 740 350 835 350", "cyan", "状态"]])}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: keys.map((key) => ({
          at: 10,
          selector: `#rf-${key} .reg-value`,
          text: String(flagBits[key]),
        })),
      },
    ),
  );
  reveal(s.timeline, s.$("#rf-head"), 0.1);
  reveal(s.timeline, s.$("#rf-c"), 0.6);
  reveal(s.timeline, s.$("#rf-cmp"), 1.5);
  flow(s, "rf-compare", 2.5, 1.7);
  keys.forEach((key) => {
    reveal(s.timeline, s.$("#rf-" + key), 4.3);
    commit(s, `#rf-${key} .reg-value`, flagBits[key], 4.4);
  });
  reveal(s.timeline, s.$("#rf-rflags"), 5.5);
  reveal(s.timeline, s.$("#rf-note"), 8);
  return closeShot(s);
}
mountOS({
  id: "os-registers",
  title: "x86-64 寄存器：参数、栈、控制与标志",
  actorContinuity: [
    actor('call-rip','register',[['regs-call','#rc-rip'],['regs-arguments','#ra-rip']]),
    actor('call-rsp','register',[['regs-call','#rc-rsp'],['regs-arguments','#ra-rsp']]),
  ],
  labLocations: {
    source: 3,
    "os_add-entry": 24,
    "os_add-returned": 43,
    "os_width-entry": 56,
    "os_equal-entry": 73,
  },
  scenes: [
    { title: "通用寄存器与调用约定", view: "X86-64 / SYSV", build: bank },
    { title: "RIP、RSP 与 call", view: "ACTUAL CALL STACK", build: call },
    {
      title: "参数与整数返回值",
      view: "ACTUAL ABI DATA",
      build: argumentsAndReturn,
    },
    { title: "RAX 与 EAX 的宽度", view: "ACTUAL REGISTER WIDTH", build: width },
    { title: "cmp 后的状态标志", view: "ACTUAL FLAGS", build: flags },
  ],
});
