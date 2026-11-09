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
  showMemory,
  actualCode,
  commit,
  osShot,
  mountOS,
} from "../shared/os-components.js";
import {
  osTrace as t,
  loadSegments,
  loadBias,
  secondBias,
  hexAddress,
  entry,
  interpreter,
  mainEntry,
  secondEntry,
  ready,
  object,
  symbol,
  byteHex,
  interpreterPath,
} from "../models/os-basics.js";
const flags = (p) =>
  `${p.flags & 4 ? "R" : "-"}${p.flags & 2 ? "W" : "-"}${p.flags & 1 ? "X" : "-"}`;
const segmentRows = (base) =>
  loadSegments
    .slice()
    .reverse()
    .map((p, i) => ({
      key: String(p.index),
      label: `PT_LOAD ${flags(p)}`,
      address: hexAddress(BigInt(base) + BigInt(p.vaddr)),
      value: `${p.memorySize}B`,
    }));
function segments() {
  const rows = loadSegments.map((p) => ({
    label: `PT_LOAD ${flags(p)}`,
    detail: `file offset ${hexAddress(p.offset)}`,
    value: `${p.fileSize}B`,
  }));
  const routes = loadSegments.map((p, i) => [
    `seg-map-${i}`,
    `M650 ${310 + i * 80}C755 ${310 + i * 80} 755 ${550 - i * 80} 860 ${550 - i * 80}`,
    "cyan",
    "映射",
  ]);
  const s = osShot(
    shot(
      "load-segments",
      `<div class="camera">${heading("ls-head", 80, 100, "PROGRAM HEADERS / EXEC", "装载时，主要看段，不是把所有节搬进 RAM。")}${memory("ls-file", 80, 225, rows, { file: true, width: 550, title: "磁盘 ELF / program headers", direction: "文件 offset ↓ · 关键 PT_LOAD", cellHeight: 80 })}${memory("ls-virtual", 880, 225, segmentRows(loadBias), { width: 460, title: `进程 PID ${entry.pid}`, direction: "虚拟 VA ↑ · 逻辑映射，不是物理 RAM", cellHeight: 80, staged: true })}${note("ls-note", 120, 635, "p_filesz 与 p_memsz 可以不同", "权限来自段；运行时还可能被 RELRO 等细化。节用于链接，段用于装载。", { width: 1160 })}${wires("ls-wires", routes)}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [{ at: 10, selector: "#ls-virtual", shown: true }],
      },
    ),
  );
  reveal(s.timeline, s.$("#ls-head"), 0.1);
  showMemory(s, "ls-file", 0.6);
  showMemory(s, "ls-virtual", 1.2);
  routes.forEach(([key], i) => {
    flow(s, key, 2 + i * 1.4, 1);
    reveal(
      s.timeline,
      s.$(`#ls-virtual .array-row:nth-child(${loadSegments.length - i + 1})`),
      3.1 + i * 1.4,
      { y: 0, duration: 0.35 },
    );
  });
  reveal(s.timeline, s.$("#ls-note"), 9);
  return closeShot(s);
}
function bias() {
  const main = symbol("executable", "main");
  const s = osShot(
    shot(
      "load-bias",
      `<div class="camera">${heading("lb-head", 80, 100, "PIE / ACTUAL LOAD BIAS", "链接时地址，怎样变成运行时地址？")}${register("lb-link", 80, 265, "ELF 中 main 的链接时 VA", main.value, { width: 530 })}${register("lb-bias", 80, 415, "load bias / 本次装载偏移", loadBias, { width: 600 })}${register("lb-live", 80, 545, "GDB / 运行时 &main", "—", { width: 600 })}${memory("lb-mapping",880,225,segmentRows(loadBias),{width:460,title:`进程 PID ${entry.pid}`,direction:"虚拟 VA ↑ · 同一次执行的运行映射",cellHeight:80})}${pos("lb-formula", 80, 700, "runtime VA = load bias + ELF VA", "os-formula", { width: 1170 })}${note("lb-note", 880, 640, "偏移来自实际 AT_ENTRY 与 e_entry", "同一PID，映射保留；main入口可在GDB核对。", { width: 460 })}${wires("lb-wires", [["lb-add", "M695 475H735V600H695", "cyan", "相加"]])}</div>`,
      16,
      {
        stops: [10.5, 14.5],
        checks: [
          {
            at: 12,
            selector: "#lb-live .reg-value",
            text: mainEntry.registers.rip,
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#lb-head"), 0.1);
  showMemory(s, "lb-mapping", 0.6);
  reveal(s.timeline, s.$("#lb-link"), 0.7);
  reveal(s.timeline, s.$("#lb-bias"), 2);
  reveal(s.timeline, s.$("#lb-live"), 3);
  flow(s, "lb-add", 4, 2);
  commit(s, "#lb-live .reg-value", mainEntry.registers.rip, 6.1);
  reveal(s.timeline, s.$("#lb-formula"), 7.5);
  reveal(s.timeline, s.$("#lb-note"), 9);
  return closeShot(s);
}
function zero() {
  const data = object(ready, "data"),
    bss = object(ready, "bss");
  const bytes = (o) =>
    o.bytes
      .map((v, i) => ({
        key: String(i),
        label: `byte[${i}]`,
        address: hexAddress(BigInt(o.address) + BigInt(i)),
        value: byteHex(v),
      }))
      .reverse();
  const zeros = bytes(bss).map((row) => ({ ...row, value: "—" }));
  const s = osShot(
    shot(
      "load-zero",
      `<div class="camera">${heading("lz-head", 80, 100, "INITIAL CONTENT / NOBITS", "带初值的数据，与零初始化区域。")}${actualCode("lz-source", 80, 265, "os-math.c", ["int os_data = 10;", "int os_bss;"], { width: 530 })}${memory("lz-data", 720, 245, bytes(data), { width: 290, title: ".data / os_data", direction: "实际 VA ↑ · 小端字节", cellHeight: 66 })}${memory("lz-bss", 1045, 245, zeros, { width: 290, title: ".bss / os_bss", direction: "实际 VA ↑ · 原始值为零", cellHeight: 66 })}${note("lz-note", 80, 475, "SHT_NOBITS 不保存这些零字节", "装载语义提供初始零值，运行后当然可以改变；未初始化的自动栈变量不是同一回事。", { width: 580 })}${register("lz-bias",80,640,"load bias / 同一次装载",loadBias,{width:600})}${pos("lz-observed", 745, 625, `GDB layout-ready：data=${data.value}，bss=${bss.value}`, "os-small", { width: 590 })}${wires("lz-wires", [["lz-fill", "M665 430C690 430 690 600 775 600H1025V529", "cyan", "零值"]])}</div>`,
      14,
      {
        stops: [9.5, 12.5],
        checks: [
          { at: 10, selector: "#lz-bss-0 .array-value", text: "00" },
          {
            at: 10,
            selector: "#lz-data-0 .array-value",
            text: byteHex(data.bytes[0]),
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#lz-head"), 0.1);
  reveal(s.timeline, s.$("#lz-bias"), 0.6);
  reveal(s.timeline, s.$("#lz-source"), 0.7);
  showMemory(s, "lz-data", 1.5);
  showMemory(s, "lz-bss", 2);
  flow(s, "lz-fill", 3, 1.5);
  bss.bytes.forEach((v, i) =>
    commit(s, `#lz-bss-${i} .array-value`, byteHex(v), 4.6 + i * 0.55),
  );
  reveal(s.timeline, s.$("#lz-note"), 7.2);
  reveal(s.timeline, s.$("#lz-observed"), 8.2);
  return closeShot(s);
}
function startup() {
  const s = osShot(
    shot(
      "load-startup",
      `<div class="camera">${heading("st-head", 80, 100, "INTERPRETER → _START → MAIN", "main，并不是第一条用户指令。")}${pos("st-path", 80, 210, `PT_INTERP = ${interpreterPath}`, "os-small", { width: 1160 })}${pos("st-review",80,250,"回看较早入口：不把layout-ready变量带回main入口。","os-small",{width:1100})}${register("st-interp", 90, 290, "%rip / 动态解释器", interpreter.registers.rip, { width: 380 })}${register("st-app", 530, 420, "%rip / 应用 _start", "—", { width: 390 })}${register("st-main", 970, 550, "%rip / main 入口", "—", { width: 380 })}${note("st-note", 80, 590, "内核先转交解释器，启动代码再调用 main", "动态链接器处理必要的动态链接；C 运行库完成启动。PLT 也可能采用延迟绑定。", { width: 760 })}${wires(
        "st-wires",
        [
          ["st-to-app", "M475 350C500 350 495 480 515 480", "cyan", "转交"],
          ["st-to-main", "M925 480C950 480 935 610 955 610", "cyan", "调用"],
        ],
      )}</div>`,
      20,
      {
        stops: [11.5, 17.5],
        checks: [
          {
            at: 14,
            selector: "#st-main .reg-value",
            text: mainEntry.registers.rip,
          },
        ],
      },
    ),
  );
  reveal(s.timeline, s.$("#st-head"), 0.1);
  reveal(s.timeline, s.$("#st-review"), 0.2);
  reveal(s.timeline, s.$("#st-path"), 0.6);
  reveal(s.timeline, s.$("#st-interp"), 1);
  flow(s, "st-to-app", 3, 2);
  reveal(s.timeline, s.$("#st-app"), 5.1);
  commit(s, "#st-app .reg-value", entry.registers.rip, 5.15);
  flow(s, "st-to-main", 7, 2);
  reveal(s.timeline, s.$("#st-main"), 9.1);
  commit(s, "#st-main .reg-value", mainEntry.registers.rip, 9.15);
  reveal(s.timeline, s.$("#st-note"), 11.5);
  return closeShot(s);
}
function aslr() {
  const s = osShot(
    shot(
      "load-aslr",
      `<div class="camera">${heading("ar-head", 80, 100, "TWO INDEPENDENT EXECUTIONS", "同一 ELF，可以装到不同的虚拟地址。")}${memory("ar-one", 80, 250, segmentRows(loadBias), { width: 510, title: `exec #1 · PID ${entry.pid}`, direction: "本次实测 VA ↑ · 不推算物理页号", cellHeight: 70 })}${memory("ar-two", 850, 250, segmentRows(secondBias), { width: 510, title: `exec #2 · PID ${secondEntry.pid}`, direction: "独立运行 VA ↑ · 相同 ELF 哈希", cellHeight: 70 })}${note("ar-note", 180, 635, "建立映射，不等于一次性读入全部物理 RAM", "按需驻留是 OS 机制解释；本片没有用 GDB 证明所有物理页的装入时刻。", { width: 1080 })}${wires("ar-wires", [["ar-compare", "M610 440H835", "blue", "对照"]])}</div>`,
      16,
      {
        stops: [10.5, 14.5],
        checks: [{ at: 11, selector: "#ar-two", shown: true }],
      },
    ),
  );
  reveal(s.timeline, s.$("#ar-head"), 0.1);
  showMemory(s, "ar-one", 0.7);
  flow(s, "ar-compare", 2, 2);
  showMemory(s, "ar-two", 4.1);
  reveal(s.timeline, s.$("#ar-note"), 8.5);
  return closeShot(s);
}
mountOS({
  id: "os-loading",
  title: "exec 与装载：ELF 如何进入进程",
  actorContinuity: [
    actor('loaded-segments','array',[['load-segments','#ls-virtual'],['load-bias','#lb-mapping']]),
    actor('same-load-bias','register',[['load-bias','#lb-bias'],['load-zero','#lz-bias']]),
  ],
  labLocations: {
    source: 3,
    "executable-segments": 8,
    "executable-header": 22,
    "interpreter-entry": 47,
    "app-entry": 53,
    "main-entry": 59,
    "layout-ready": 39,
  },
  scenes: [
    { title: "文件段与逻辑映射", view: "PT_LOAD / MAPPING", build: segments },
    { title: "PIE 与实际装载偏移", view: "ACTUAL LOAD BIAS", build: bias },
    { title: "data 与 bss 的初始内容", view: "INITIAL BYTES", build: zero },
    {
      title: "解释器、_start 与 main",
      view: "ACTUAL STARTUP STOPS",
      build: startup,
    },
    { title: "两次 exec 与按需驻留", view: "ASLR + MODEL", build: aslr },
  ],
});
