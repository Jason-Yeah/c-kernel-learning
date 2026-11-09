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
  code,
} from "../shared/scene-kit.js";
import {
  memory,
  file,
  showMemory,
  commit,
  osShot,
  mountOS,
} from "../shared/os-components.js";
import {
  osTrace as t,
  section,
  symbol,
  byteHex,
  hexAddress,
} from "../models/os-basics.js";
function header() {
  const h = t.elf.main.header;
  const rows = [
    {
      label: "ELF Header",
      detail: "ELF64 · little-endian",
      value: `${h.size}B`,
    },
    { label: "e_type", detail: "可重定位目标文件", value: "ET_REL" },
    { label: "e_machine", detail: "ISA / x86-64", value: String(h.machine) },
    {
      label: "e_shoff",
      detail: "节头表的文件偏移",
      value: hexAddress(h.shoff),
    },
    { label: "e_shnum", detail: "真实节头数量", value: String(h.shnum) },
    { label: "e_phnum", detail: "本 .o 没有装载段表", value: String(h.phnum) },
  ];
  const s = osShot(
    shot(
      "elf-header",
      `<div class="camera">${heading("eh-head", 80, 100, "RELOCATABLE / NOT REDIRECTION", "可重定位 .o：先是一份磁盘文件。")}${file("eh-file", 100, 320, "os-main.o", "可重定位 ELF 文件")}${memory("eh-array", 810, 225, rows, { file: true, width: 520, title: "ELF64 文件头 / 实际字段", direction: "磁盘文件 · offset ↓ 增大", cellHeight: 56, staged: true })}${note("eh-note", 80, 580, "不是 shell 的 > 输出重定向", "这里“重定位”指链接器调整符号引用；不是把 stdout 改到文件。", { width: 620 })}${wires("eh-wires", [["eh-open", "M365 415H795", "cyan", "解析"]])}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [
          {
            at: 10,
            selector: "#eh-array-4 .array-value",
            text: String(h.shnum),
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#eh-head"), 0.1);
  reveal(s.timeline, s.$("#eh-file"), 0.6);
  flow(s, "eh-open", 1.7, 1.7);
  showMemory(s, "eh-array", 3.5, { staged: true, interval: 0.6 });
  reveal(s.timeline, s.$("#eh-note"), 8.5);
  return closeShot(s);
}
function sections() {
  const items = [
    ".text",
    ".data",
    ".bss",
    ".rodata",
    ".symtab",
    ".rela.text",
  ].map((name) => section("main", name));
  const rows = items.map((s) => ({
    label: s.name,
    detail: `file ${hexAddress(s.offset)} · ${s.type === 8 ? "NOBITS" : "节内容"}`,
    value: s.type === 8 ? `文件0 / 逻辑${s.size}B` : `${s.size}B`,
  }));
  const s = osShot(
    shot(
      "elf-sections",
      `<div class="camera">${heading("es-head", 80, 100, "SECTION VIEW / LINKER", "节：按代码、数据与链接信息分类。")}${memory("es-array", 80, 225, rows, { file: true, title: "os-main.o / 关键节折叠视图", direction: "文件 offset · 不按大小比例 · 其他节省略", width: 570, cellHeight: 61, staged: true })}${file("es-file",800,220,"os-main.o","同一个目标文件",{width:200})}${note("es-text", 1020, 250, ".text / .data / .rodata", "代码、带初值的可写数据和只读数据，各自有实际字节。", { width: 320 })}${note("es-bss", 1020, 420, ".bss = SHT_NOBITS", "有逻辑大小，不为初始零值存同样大小的文件字节。", { width: 320 })}${note("es-meta", 850, 595, ".symtab / .rela.text", "给链接器看的符号和重定位信息；不是所有节都会装入进程。", { width: 470 })}</div>`,
      16,
      {
        stops: [10.5, 14.5],
        checks: [
          {
            at: 11,
            selector: "#es-array-2 .array-value",
            text: `文件0 / 逻辑${section("main", ".bss").size}B`,
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#es-head"), 0.1);
  reveal(s.timeline, s.$("#es-file"), 0.6);
  showMemory(s, "es-array", 0.6, { staged: true, interval: 0.8 });
  reveal(s.timeline, s.$("#es-text"), 3.1);
  reveal(s.timeline, s.$("#es-bss"), 5.5);
  reveal(s.timeline, s.$("#es-meta"), 8.5);
  return closeShot(s);
}
function symbols() {
  const undefAdd = symbol("main", "os_add"),
    defAdd = symbol("math", "os_add"),
    undefData = symbol("main", "os_data"),
    defData = symbol("math", "os_data");
  const left = [
    { label: "os_add", detail: "main.o 的外部引用", value: undefAdd.section },
    { label: "os_data", detail: "main.o 的外部引用", value: undefData.section },
  ];
  const right = [
    {
      label: "os_add",
      detail: `${defAdd.section} + ${defAdd.value}`,
      value: "定义",
    },
    {
      label: "os_data",
      detail: `${defData.section} + ${defData.value}`,
      value: "定义",
    },
  ];
  const s = osShot(
    shot(
      "elf-symbols",
      `<div class="camera">${heading("sy-head", 80, 100, "SYMBOL / DEFINITION AND REFERENCE", "未定义，不等于代码有错。")}${memory("sy-left", 90, 300, left, { file: true, width: 460, title: "os-main.o / 符号引用", direction: "UND = 此翻译单元尚无定义", cellHeight: 90 })}${memory("sy-right", 890, 300, right, { file: true, width: 460, title: "os-math.o / 可用定义", direction: "值是所在节内偏移，不是进程 VA", cellHeight: 90 })}${file("sy-file",620,220,"os-main.o","同一个目标文件",{width:200})}${note("sy-note", 325, 600, "链接器在其他目标文件／库里找定义", "本例 os_add 与 os_data 在 math.o 中找到；最终符号值见可执行 ELF。", { width: 790 })}${wires(
        "sy-wires",
        [
          ["sy-resolve", "M565 395H875", "cyan", "符号"],
          ["sy-resolve-data", "M565 485H875", "cyan", "符号"],
        ],
      )}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [{ at: 10, selector: "#sy-left-0 .array-value", text: "UND" }],
      },
    ),
  );
  reveal(s.timeline, s.$("#sy-head"), 0.1);
  reveal(s.timeline, s.$("#sy-file"), 0.6);
  showMemory(s, "sy-left", 0.6);
  showMemory(s, "sy-right", 2);
  flow(s, "sy-resolve", 3.2, 1.5);
  flow(s, "sy-resolve-data", 5.3, 1.5);
  reveal(s.timeline, s.$("#sy-note"), 8);
  return closeShot(s);
}
function relocation() {
  const r = t.relocation,
    displacement = BigInt(r.S) + BigInt(r.A) - BigInt(r.P);
  const rows = r.before
    .map((b, i) => ({
      key: String(i),
      label: `字段 byte[${i}]`,
      address: hexAddress(BigInt(r.P) + BigInt(i)),
      value: byteHex(b),
    }))
    .reverse();
  const s = osShot(
    shot(
      "elf-relocation",
      `<div class="camera">${heading("rel-head", 80, 100, "R_X86_64_PC32 / ACTUAL LINKER PATCH", "链接器，真正修改了哪四个字节？")}${pos("rel-formula", 80, 240, `S + A − P = ${displacement}<br><span class="os-small">S=${r.S} · A=${r.A} · P=${r.P}</span>`, "os-formula", { width: 720 })}${memory("rel-bytes", 900, 245, rows, { width: 420, title: "最终 ELF 中的重定位字段", direction: "链接时 VA ↑ · 此处不是运行 VA", cellHeight: 67 })}${register("rel-target", 80, 405, "RIP 相对引用的目标", "—", { width: 640 })}${note("rel-note", 80, 575, "P 指四字节字段本身，不是指令开始", "本例 addend=-4；CPU 用后继指令地址 P+4 加位移，最终指向 os_data。", { width: 735 })}${wires(
        "rel-wires",
        [
          ["rel-patch", "M790 360H885", "cyan", "补丁"],
          ["rel-use", "M885 495C820 495 800 460 750 460", "blue", "引用"],
        ],
      )}</div>`,
      20,
      {
        stops: [10.5, 16.5, 18.5],
        checks: r.after
          .map((v, i) => ({
            at: 17,
            selector: `#rel-bytes-${i} .array-value`,
            text: byteHex(v),
          }))
          .concat([{ at: 17, selector: "#rel-target .reg-value", text: r.S }]),
      },
    ),
  );
  reveal(s.timeline, s.$("#rel-head"), 0.1);
  reveal(s.timeline, s.$("#rel-formula"), 0.7);
  showMemory(s, "rel-bytes", 1.3);
  flow(s, "rel-patch", 3, 1.8);
  r.after.forEach((v, i) =>
    commit(s, `#rel-bytes-${i} .array-value`, byteHex(v), 5 + i * 0.65),
  );
  reveal(s.timeline, s.$("#rel-target"), 8);
  flow(s, "rel-use", 9, 1.6);
  commit(s, "#rel-target .reg-value", r.S, 10.7);
  reveal(s.timeline, s.$("#rel-note"), 12);
  return closeShot(s);
}
mountOS({
  id: "os-elf",
  title: "可重定位 ELF：节、符号与重定位",
  actorContinuity: [actor('same-relocatable-file','file',[
    ['elf-header','#eh-file'],['elf-sections','#es-file'],['elf-symbols','#sy-file']
  ])],
  labLocations: {
    source: 3,
    "object-header": 8,
    "object-sections": 24,
    "math-symbols": 35,
    "object-symbols": 35,
    "object-relocations": 55,
  },
  scenes: [
    { title: "目标文件与 ELF 头", view: "ELF HEADER", build: header },
    {
      title: "节的竖向文件视图",
      view: "SECTIONS / NOT SEGMENTS",
      build: sections,
    },
    { title: "符号定义与引用", view: "SYMBOLS", build: symbols },
    {
      title: "PC32 的四字节补丁",
      view: "ACTUAL RELOCATION",
      build: relocation,
    },
  ],
});
