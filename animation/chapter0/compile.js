import { actor } from "../shared/actor-continuity.js";
import {
  shot,
  closeShot,
  heading,
  note,
  code,
  pos,
  wires,
  flow,
  reveal,
  vanish,
} from "../shared/scene-kit.js";
import {
  file,
  memory,
  showMemory,
  actualCode,
  osShot,
  mountOS,
} from "../shared/os-components.js";
import {
  osTrace as t,
  byteHex,
  sourceRows,
  symbol,
} from "../models/os-basics.js";
function sources() {
  const s = osShot(
    shot(
      "compile-sources",
      `<div class="camera">${heading("src-head", 80, 100, "SOURCE / TRANSLATION UNITS", "程序文件，还不是一个进程。")}${file("src-main", 100, 315, "os-main.c", "main 与外部引用")}${file("src-header", 560, 230, "os-demo.h", "声明与宏")}${file("src-math", 1000, 315, "os-math.c", "函数与全局定义")}${note("src-note", 535, 500, "两个翻译单元，链接成一个程序", "头文件被预处理器读取；原文件不会被搬走。", { width: 370 })}${wires(
        "src-wires",
        [
          ["header-left", "M560 325C450 325 470 405 360 405", "blue", "声明"],
          ["header-right", "M820 325C930 325 910 405 1000 405", "blue", "声明"],
        ],
      )}</div>`,
      12,
      {
        stops: [7.5, 10.5],
        checks: [
          { at: 8, selector: "#src-main", shown: true },
          { at: 0.1, selector: "#src-math", shown: false },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#src-head"), 0.1);
  reveal(s.timeline, s.$("#src-main"), 0.6);
  reveal(s.timeline, s.$("#src-math"), 1);
  reveal(s.timeline, s.$("#src-header"), 2);
  flow(s, "header-left", 3, 1.5);
  flow(s, "header-right", 4.5, 1.5);
  reveal(s.timeline, s.$("#src-note"), 6.8);
  return closeShot(s);
}
function preprocess() {
  const rows = sourceRows("os-main.c", [
    '#include "os-demo.h"',
    "os_result = os_add(OS_LEFT, OS_RIGHT);",
  ]);
  const pp = t.build.preprocessed;
  const s = osShot(
    shot(
      "compile-preprocess",
      `<div class="camera">${heading("pp-head", 80, 100, "GCC -E / ACTUAL PREPROCESSOR", "先展开头文件和宏。")}${code("pp-before", 80, 270, rows.lines, { width: 570, title: "os-main.c / 真实源码", lineNumbers: rows.numbers })}${code(
        "pp-after",
        830,
        300,
        pp.map((r) => r.text),
        {
          width: 530,
          title: "os-main.i / 实际输出",
          lineNumbers: pp.map((r) => r.line),
        },
      )}${file("pp-main", 80, 470, "os-main.c", "原始源文件仍保留")}${pos("pp-command", 400, 525, "gcc -E os-main.c<br>→ os-main.i", "os-mid", {width:360})}${note("pp-note", 830, 540, "OS_LEFT / OS_RIGHT → 3 / 2", "这里仍是 C 文本，不是 CPU 指令。完整 .i 可在实验台下载。", { width: 510 })}${wires("pp-wires", [["pp-expand", "M655 380H820", "cyan", "展开"]])}</div>`,
      12,
      {
        stops: [7.5, 10.5],
        checks: [
          {
            at: 8,
            selector: "#pp-after .code-line:last-child code",
            text: pp.at(-1).text,
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#pp-head"), 0.1);
  reveal(s.timeline, s.$("#pp-main"), 0.6);
  reveal(s.timeline, s.$("#pp-before"), 0.6);
  reveal(s.timeline, s.$("#pp-command"), 1.3);
  flow(s, "pp-expand", 2, 2);
  reveal(s.timeline, s.$("#pp-after"), 4.1);
  reveal(s.timeline, s.$("#pp-note"), 6);
  return closeShot(s);
}
const assemblyLines = t.build.assemblyAdd.filter(l => /^(movl\s+-|addl|leave|ret)/.test(l));
function compiler() {
  const asm = assemblyLines;
  const s = osShot(
    shot(
      "compile-assembly",
      `<div class="camera">${heading("cc-head", 80, 100, "GCC -S / 另一个翻译单元 os-math.c", "把 C 运算，编译成汇编指令。")}${actualCode("cc-c", 80, 285, "os-math.c", ["int os_add(int left, int right)", "return left + right;"], { width: 530 })}${code("cc-asm", 830, 235, asm, { width: 500, title: "os-math.s / 实际运算与返回节选" })}${note("cc-note", 80, 505, "不是每一行 C 只对应一条指令", "本实验 -O0、保留帧指针；优化后布局与指令可以变化。", { width: 540 })}${wires("cc-wires", [["cc-data", "M625 390H815", "cyan", "编译"]])}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [{ at: 10, selector: "#cc-asm", shown: true }],
      },
    ),
  );
  reveal(s.timeline, s.$("#cc-head"), 0.1);
  reveal(s.timeline, s.$("#cc-c"), 0.6);
  flow(s, "cc-data", 2, 1.7);
  reveal(s.timeline, s.$("#cc-asm"), 3.8);
  asm.forEach((_, i) =>
    s.timeline.to(
      s.$(`#cc-asm [data-line="${i}"]`),
      { color: "#69e2f4", duration: 0.3 },
      4.8 + i * 0.7,
    ),
  );
  reveal(s.timeline, s.$("#cc-note"), 8.5);
  return closeShot(s);
}
function assembler() {
  const rows = [];
  for (let i = 0; i < t.build.addBytes.length; i += 4)
    rows.push({
      key: String(i),
      label: `.text + 0x${i.toString(16).toUpperCase()}`,
      detail: "真实 os_add 机器码",
      value: t.build.addBytes
        .slice(i, i + 4)
        .map(byteHex)
        .join(" "),
    });
  const s = osShot(
    shot(
      "compile-object",
      `<div class="camera">${heading("as-head", 80, 100, "ASSEMBLER / REAL FILE BYTES", "汇编器，把指令编码成目标文件。")}${code("as-file", 80, 235, assemblyLines, {width:540,title:"os-math.s / 同一份汇编输出"})}${file("as-object",100,500,"os-math.o","刚生成的目标文件")}${memory("as-bytes", 830, 235, rows, { width: 500, title: "os-math.o / .text 中 os_add", direction: "磁盘文件 · offset ↓ 增大 · 每格4字节", file: true, cellHeight: 49, staged: true })}${note("as-note", 400, 520, "ET_REL：可重定位，不是可直接启动的程序", "机器码已存在，外部引用和最终地址仍可能等待链接。", { width: 390 })}${wires("as-wires", [["as-encode", "M650 400H815", "cyan", "编码"]])}</div>`,
      12,
      {
        stops: [8.5, 10.5],
        checks: [
          {
            at: 9,
            selector: "#as-bytes-0 .array-value",
            text: t.build.addBytes.slice(0, 4).map(byteHex).join(" "),
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#as-head"), 0.1);
  reveal(s.timeline, s.$("#as-file"), 0.6);
  flow(s, "as-encode", 1.6, 2);
  showMemory(s, "as-bytes", 3.7, { staged: true, interval: 0.38 });
  reveal(s.timeline, s.$("#as-object"), 6.7);
  reveal(s.timeline, s.$("#as-note"), 7.2);
  return closeShot(s);
}
function link() {
  const add = symbol("executable", "os_add");
  const s = osShot(
    shot(
      "compile-link",
      `<div class="camera">${heading("ld-head", 80, 100, "LINK EDIT / NOT EXECUTION", "链接，把定义和引用接起来。")}${file("ld-main", 90, 250, "os-main.o", "main：引用 os_add")}${file("ld-math", 90, 490, "os-math.o", "定义 os_add、os_data")}${file("ld-output", 970, 325, "os-demo", "ELF64 / ET_DYN / PIE")}${pos("ld-resolved", 480, 310, `<div class="os-mid">os_add</div><div class="os-small">链接后符号值</div><div class="os-mid">${add.value}</div>`, "os-band", { width: 370 })}${note("ld-note", 470, 550, "链接时 VA，不是 ASLR 后的运行地址", "下一步还需要 OS 建立进程地址空间并转交控制。", { width: 460 })}${wires(
        "ld-wires",
        [
          ["ld-reference", "M355 345C410 345 410 355 470 355", "blue", "引用"],
          ["ld-definition", "M355 585C415 585 415 420 470 420", "cyan", "定义"],
          ["ld-write", "M865 390H955", "cyan", "链接"],
        ],
      )}</div>`,
      14,
      {
        stops: [8.5, 12.5],
        checks: [
          { at: 10, selector: "#ld-output", shown: true },
          { at: 10, selector: "#ld-resolved", shown: true },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#ld-head"), 0.1);
  reveal(s.timeline, s.$("#ld-main"), 0.5);
  reveal(s.timeline, s.$("#ld-math"), 1);
  flow(s, "ld-reference", 2, 1.2);
  flow(s, "ld-definition", 3, 1.7);
  reveal(s.timeline, s.$("#ld-resolved"), 4.9);
  flow(s, "ld-write", 6, 1.3);
  reveal(s.timeline, s.$("#ld-output"), 7.4);
  reveal(s.timeline, s.$("#ld-note"), 8.5);
  return closeShot(s);
}
mountOS({
  id: "os-compile",
  title: "从 C 源码到可执行文件",
  actorContinuity: [
    actor('source-main-file','file',[['compile-sources','#src-main'],['compile-preprocess','#pp-main']]),
    actor('math-assembly','code',[['compile-assembly','#cc-asm'],['compile-object','#as-file']]),
    actor('math-object-file','file',[['compile-object','#as-object'],['compile-link','#ld-math']]),
  ],
  labLocations: {
    source: 2,
    "math-disassembly": 29,
    "object-header": 43,
    "executable-header": 58,
  },
  scenes: [
    { title: "两个翻译单元", view: "SOURCE FILES", build: sources },
    { title: "预处理", view: "GCC -E", build: preprocess },
    { title: "编译成汇编", view: "GCC -S", build: compiler },
    { title: "汇编成目标文件", view: "ELF ET_REL", build: assembler },
    { title: "链接定义与引用", view: "LINK EDITOR", build: link },
  ],
});
