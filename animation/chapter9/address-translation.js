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
  hex,
} from "../shared/scene-kit.js";
import { vmCases } from "../models/computer.js";
import { vmObservations } from "../models/experiments.js";
const coldObservation = vmObservations.cold;
function mmu(id, x, y, translation) {
  return pos(id + '-case', x, y - 40, '独立访存对照：新请求，TLB/PTE按本例初态', 'local-note', {visible:true,width:650}) + pos(
    id,
    x,
    y,
    `<svg class="mmu-shape" viewBox="0 0 520 330" aria-hidden="true"><path d="M20 12H500V318H20Z"/><g>${[75, 130, 185, 240].map((p) => `<path d="M2 ${p}H20M500 ${p}H518"/>`).join("")}</g></svg><div class="mmu-name">CPU <span>/ MMU</span></div><div class="mmu-tlb" id="${id}-tlb" data-anim><div class="code-heading">TLB / 翻译与权限缓存</div><div class="tlb-row"><span class="translation">${translation}</span></div></div><div class="mmu-foot">同一颗 CPU · 不是两台设备</div>`,
    "mmu-component",
    { width: 520 },
  );
}
function bitsIntro() {
  const s = shot(
    "vm-address-bits",
    `<div class="camera">${pos("vm-origin", 570, 270, '<div class="address-value" style="font-size:90px">0x12D</div><div class="eyebrow" style="margin-top:18px">A VIRTUAL ADDRESS</div>', "", { visible: true })}
    ${pos("vm-bit-fields", 330, 345, '<div class="bit-fields"><div class="bit-field"><b>000100</b><small>VPN = 04 / 6 bit</small></div><div class="bit-field offset"><b>101101</b><small>OFFSET = 2D / 6 bit</small></div></div>')}
    ${code("vm-read-code", 160, 550, ["uint8_t value = *p;", "// VA 12D：页号 04，页内偏移 2D"], { width: 650 })}${note("vm-bit-note", 950, 370, "偏移保持不变", "页大小为 64B；这是缩小的教学模型。", { width: 330 })}</div>`,
    7,
    {
      stops: [3.7, 6],
      checks: [{ at: 4.7, selector: "#vm-bit-fields", shown: true }],
    },
  );
  s.timeline.to(
    s.$("#vm-origin"),
    { x: -400, y: -135, scale: 0.68, transformOrigin: "0 0", duration: 1.2 },
    0.4,
  );
  reveal(s.timeline, s.$("#vm-bit-fields"), 1.8, { y: 0, scale: 0.96 });
  reveal(s.timeline, s.$("#vm-read-code"), 2.5);
  activeCode(s, 0, 3);
  reveal(s.timeline, s.$("#vm-bit-note"), 4.5);
  return closeShot(s);
}
function tlbHit() {
  const c = vmCases[0];
  const s = shot(
    "vm-tlb-hit",
    `<div class="camera">${register("hit-VA", 170, 130, "VA / READ", `0x${hex(c.va, 3)}`, { width: 340, visible: true })}${register("hit-PA", 930, 130, "PA / 页号替换，偏移不变", "—", { width: 360 })}
    ${mmu("hit-mmu", 170, 305, "04 → 09 · R/W")}${page("hit-page", 1070, 320, "RAM / PAGE 09", hex(c.data))}${register("hit-AL", 910, 635, "CPU / AL", "—", { width: 250 })}
    ${pos("hit-offset", 680, 170, '<span class="mono" style="color:#9fbdff;font-size:26px">OFFSET 2D</span>')}${note("hit-permission", 770, 535, "命中，也要检查读权限", "只缓存翻译与权限，不缓存这次数据。", { width: 550 })}
    ${wires("hit-wires", [
      ["hit-query", "M155 210H145V430H200", "cyan"],
      ["hit-map", "M670 465C780 465 770 210 910 210", "blue", "09"],
      ["hit-data-read", "M700 490C880 490 910 395 1050 395", "blue"],
      ["hit-data-return", "M1165 515C1165 610 1120 690 1177 690", "cyan", "7B"],
    ])}</div>`,
    12,
    {
      stops: [3.6, 6.5, 10.6],
      checks: [
        { at: 5.5, selector: "#hit-PA .reg-value", text: "—" },
        { at: 7, selector: "#hit-PA .reg-value", text: `0x${hex(c.pa, 3)}` },
        { at: 10.7, selector: "#hit-AL .reg-value", text: hex(c.data) },
      ],
    },
  );
  reveal(s.timeline, s.$("#hit-mmu"), 0.1);
  reveal(s.timeline, s.$("#hit-PA"), 0.2);
  reveal(s.timeline, s.$("#hit-AL"), 0.3);
  flow(s, "hit-query", 0.7, 1);
  reveal(s.timeline, s.$("#hit-mmu-tlb"), 1.8);
  reveal(s.timeline, s.$("#hit-offset"), 2.5);
  reveal(s.timeline, s.$("#hit-permission"), 3.1);
  flow(s, "hit-map", 4.2, 1.1);
  number(s.timeline, s.$("#hit-PA .reg-value"), `0x${hex(c.pa, 3)}`, 5.6);
  state(s, "translation-authorized", 5.6);
  reveal(s.timeline, s.$("#hit-page"), 6.1);
  flow(s, "hit-data-read", 6.8, 1.1);
  flow(s, "hit-data-return", 8.7, 1.1);
  number(s.timeline, s.$("#hit-AL .reg-value"), hex(c.data), 9.9);
  state(s, "data-returned", 9.9);
  return closeShot(s);
}
function residentMiss() {
  const c = vmCases[1];
  const s = shot(
    "vm-resident-miss",
    `<div class="camera">${register("miss-VA", 170, 130, "VA / READ", `0x${hex(c.va, 3)}`, { width: 340, visible: true })}${register("miss-PA", 930, 125, "PA", "—", { width: 350 })}
    ${mmu("miss-mmu", 170, 300, "05 → MISS")}${windowBox("miss-PTE", 855, 290, "RAM / PAGE TABLE", "<code>05 → 02 · P=1</code><small>R/W · 页面已经驻留</small>", { width: 430 })}${page("miss-data-page", 1080, 490, "RAM / PAGE 02", hex(c.data), { width: 220 })}
    ${register("miss-AL", 170, 650, "CPU / AL", "—", { width: 250 })}${note("miss-note", 750, 590, "TLB miss，不等于缺页", "硬件继续读页表；全程仍在用户态。", { width: 300 })}
    ${wires("miss-wires", [
      ["miss-query", "M155 210H145V430H200", "cyan"],
      ["miss-walk", "M700 475C785 475 760 365 835 365", "blue"],
      ["miss-refill", "M840 410C760 410 760 475 705 475", "cyan", "02"],
      ["miss-page-read", "M1190 455V472", "blue"],
      ["miss-data-return", "M1060 595C850 650 690 705 438 705", "cyan", "A6"],
    ])}</div>`,
    12,
    {
      stops: [2.1, 4.1, 6.2, 10.2],
      checks: [
        { at: 2.1, selector: "#miss-mmu .translation", text: "05 → MISS" },
        { at: 6, selector: "#miss-mmu .translation", text: "05 → 02 · R/W" },
        { at: 10.5, selector: "#miss-AL .reg-value", text: hex(c.data) },
      ],
    },
  );
  ["#miss-mmu", "#miss-PA", "#miss-AL"].forEach((e) =>
    reveal(s.timeline, s.$(e), 0.15),
  );
  flow(s, "miss-query", 0.6, 0.8);
  reveal(s.timeline, s.$("#miss-mmu-tlb"), 1.5);
  reveal(s.timeline, s.$("#miss-PTE"), 2.1);
  flow(s, "miss-walk", 2.3, 1);
  reveal(s.timeline, s.$("#miss-note"), 3.7);
  flow(s, "miss-refill", 4.3, 1);
  number(s.timeline, s.$("#miss-mmu .translation"), "05 → 02 · R/W", 5.5);
  state(s, "tlb-refilled", 5.5);
  number(s.timeline, s.$("#miss-PA .reg-value"), `0x${hex(c.pa, 3)}`, 6.3);
  reveal(s.timeline, s.$("#miss-data-page"), 6.6);
  flow(s, "miss-page-read", 7, 0.8);
  flow(s, "miss-data-return", 8.2, 1.1);
  number(s.timeline, s.$("#miss-AL .reg-value"), hex(c.data), 9.5);
  return closeShot(s);
}
function legalFault() {
  const c = vmCases[2];
  const s = shot(
    "vm-legal-fault",
    `<div class="camera">${register("page-VA", 170, 125, "VA / READ", `0x${hex(c.va, 3)}`, { width: 330, visible: true })}${mmu("page-mmu", 170, 300, "06 → MISS")}
    ${windowBox("page-PTE", 895, 155, "RAM / PAGE TABLE", "<code>06 → — · P=0</code><small>驻留位不是映射合法性的全部信息</small>", { width: 425 })}${page("page-zero", 1080, 345, "FRAME 0B / ZERO PAGE", "…")}${pos("page-zero-stream", 1102, 430, "00 00 00", "mono zero-stream")}${register("page-PA", 760, 365, "PA", "—", { width: 240 })}${register("page-AL", 170, 650, "CPU / AL", "—", { width: 240 })}
    ${pos("page-kernel-mode", 470, 665, "同一 CPU / 内核态", "kernel-mode")}${pos("page-valid-region", 760, 90, "实机 cold 访问前：present=0，映射允许读写", "tiny-note", {width:570})}
    ${code("page-kernel-code", 760, 550, ["check_mapping(va);", "frame = alloc_zero_page();", "update_pte(frame, PRESENT);", "return_to_user_and_retry();"], { width: 570, title: "内核机制伪代码 / 不是本机内核源码" })}
    ${wires("page-wires", [
      ["page-query", "M155 210H145V430H200", "cyan"],
      ["page-walk", "M700 475C815 475 780 238 875 238", "blue"],
      ["page-P0", "M875 265C770 265 800 475 705 475", "blue", "P=0"],
      ["page-alloc", "M1140 535V515", "cyan"],
      ["page-retry", "M700 500C845 500 780 235 875 235", "blue"],
      ["page-refill", "M875 270C780 270 790 470 705 470", "cyan", "0B"],
      ["page-read", "M1015 420H1063", "blue"],
      ["page-return", "M1060 465C920 640 770 705 438 705", "cyan", "00"],
    ])}</div>`,
    23,
    {
      stops: [4.3, 6.9, 11.7, 13.6, 17.4, 21.1],
      checks: [
        { at: 6, selector: "#page-zero", shown: false },
        { at: 8.3, selector: "#page-zero .page-value", text: "…" },
        { at: 9.9, selector: "#page-zero .page-value", text: "00" },
        { at: 10, selector: "#page-PTE code", text: "06 → — · P=0" },
        { at: 12, selector: "#page-PTE code", text: "06 → 0B · P=1" },
        { at: 12, selector: "#page-mmu .translation", text: "06 → MISS" },
        { at: 17.6, selector: "#page-mmu .translation", text: "06 → 0B · R/W" },
        { at: 21, selector: "#page-AL .reg-value", text: "00" },
      ],
    },
  );
  ["#page-mmu", "#page-AL"].forEach((e) => reveal(s.timeline, s.$(e), 0.1));
  flow(s, "page-query", 0.6, 0.8);
  reveal(s.timeline, s.$("#page-mmu-tlb"), 1.5);
  reveal(s.timeline, s.$("#page-PTE"), 2);
  flow(s, "page-walk", 2.4, 1);
  flow(s, "page-P0", 3.5, 0.85);
  reveal(s.timeline, s.$("#page-kernel-mode"), 4.5);
  state(s, "page-fault-kernel", 4.5);
  reveal(s.timeline, s.$("#page-kernel-code"), 5);
  activeCode(s, 0, 5.5);
  reveal(s.timeline, s.$("#page-valid-region"), 6.4);
  activeCode(s, 1, 7.2);
  reveal(s.timeline, s.$("#page-zero"), 8, { y: 0, scale: 0.92 });
  flow(s, "page-alloc", 8.3, 0.8);
  s.timeline.set(s.$('#page-zero .page-value'),{autoAlpha:0},8.65);
  reveal(s.timeline, s.$("#page-zero-stream"), 8.7, { y: 0, duration: 0.15 });
  s.timeline.fromTo(
    s.$("#page-zero-stream"),
    { clipPath: "inset(0 100% 0 0)" },
    {
      clipPath: "inset(0 0% 0 0)",
      duration: 0.8,
      ease: "none",
      immediateRender: false,
    },
    8.7,
  );
  vanish(s.timeline, s.$("#page-zero-stream"), 9.55, { y: 0, duration: 0.15 });
  number(s.timeline, s.$("#page-zero .page-value"), "00", 9.7);
  s.timeline.set(s.$('#page-zero .page-value'),{autoAlpha:1},9.7);
  state(s, "zero-page-allocated-and-cleared", 9.7);
  activeCode(s, 2, 10.4);
  number(s.timeline, s.$("#page-PTE code"), "06 → 0B · P=1", 11);
  state(s, "pte-updated-not-tlb", 11);
  activeCode(s, 3, 12.2);
  vanish(s.timeline, s.$("#page-kernel-mode"), 13, { y: 0 });
  state(s, "retry-in-user-mode", 13);
  flow(s, "page-retry", 14.1, 1);
  flow(s, "page-refill", 16, 1);
  number(s.timeline, s.$("#page-mmu .translation"), "06 → 0B · R/W", 17.2);
  state(s, "tlb-refilled-after-retry", 17.2);
  vanish(s.timeline, s.$("#page-kernel-code"), 17.2);
  reveal(s.timeline, s.$("#page-PA"), 17.7);
  number(s.timeline, s.$("#page-PA .reg-value"), `0x${hex(c.pa, 3)}`, 18);
  flow(s, "page-read", 18.2, 0.8);
  flow(s, "page-return", 19.1, 1.1);
  number(s.timeline, s.$("#page-AL .reg-value"), "00", 20.4);
  number(
    s.timeline,
    s.$("#page-valid-region"),
    `实机 present 0→1，minor +${coldObservation.after.fields.afterFaults - coldObservation.after.fields.beforeFaults}；TLB 路径为模型`,
    20.4,
  );
  state(s, "retried-read-completed", 20.4);
  return closeShot(s);
}
function writeDenied() {
  const c = vmCases[3];
  const s = shot(
    "vm-write-denied",
    `<div class="camera">${register("denied-VA", 170, 130, "VA / WRITE FF", `0x${hex(c.va, 3)}`, { width: 350, visible: true })}${mmu("denied-mmu", 170, 300, "08 → 03 · R only")}${page("denied-physical", 1090, 340, "PAGE 03 / 原数据", "AA")}
    ${pos("denied-lock", 930, 425, '<svg class="lock-icon" viewBox="0 0 45 45"><path d="M13 20v-7a10 10 0 0 1 20 0v7M8 20h30v22H8zM23 28v7"/></svg>')}
    ${pos("denied-mode", 270, 655, "同一 CPU / 保护异常 → 内核", "kernel-mode")}${note("denied-note", 790, 565, "只读写：实测 SIGSEGV / code 2", `si_addr = ${vmObservations.readonly.fault.signal.address}；原字节 AA 未变。`, { red: true, width: 540 })}${pos("denied-result", 830, 680, "GDB：SIGSEGV / SEGV_ACCERR", "local-note")}
    ${wires("denied-wires", [
      ["denied-query", "M155 210H145V430H200", "cyan"],
      ["denied-attempt", "M700 470H910", "red", "FF"],
    ])}</div>`,
    11,
    {
      stops: [3.4, 5.5, 8.5],
      checks: [
        { at: 2, selector: "#denied-mode", shown: false },
        { at: 8.5, selector: "#denied-physical .page-value", text: "AA" },
        { at: 7, selector: "#denied-mode", shown: true },
      ],
    },
  );
  reveal(s.timeline, s.$("#denied-mmu"), 0.1);
  flow(s, "denied-query", 0.7, 0.8);
  reveal(s.timeline, s.$("#denied-mmu-tlb"), 1.7);
  reveal(s.timeline, s.$("#denied-physical"), 2.5);
  flow(s, "denied-attempt", 3.3, 1, { hide: false });
  reveal(s.timeline, s.$("#denied-lock"), 4.4, { y: 0, scale: 0.7 });
  reveal(s.timeline, s.$("#denied-mode"), 5.2);
  reveal(s.timeline, s.$("#denied-note"), 5.6);
  reveal(s.timeline, s.$("#denied-result"), 7.2);
  state(s, "write-blocked-before-target-access", 5.2);
  return closeShot(s);
}
function unmapped() {
  const c = vmCases[4];
  const s = shot(
    "vm-unmapped",
    `<div class="camera">${register("unmapped-VA", 170, 130, "VA / READ", `0x${hex(c.va, 3)}`, { width: 350, visible: true })}${mmu("unmapped-mmu", 170, 300, "07 → MISS")}
    ${windowBox("unmapped-PTE", 900, 230, "RAM / PAGE TABLE", "<code>07 → — · P=0</code><small>与合法缺页一样，先触发异常</small>", { width: 425 })}${register("unmapped-AL", 170, 650, "CPU / AL", "—", { width: 240 })}
    ${pos("unmapped-mode", 470, 665, "同一 CPU / 内核态", "kernel-mode")}${code("unmapped-code", 780, 525, ["if (!region_contains(va)) {", "    reject_access();", "} // 不分配，不回填，不读取"], { width: 540, title: "内核机制伪代码 / 合法映射检查" })}
    ${note("unmapped-note", 800, 430, "无映射：实测 SIGSEGV / code 1", `si_addr = ${vmObservations.unmapped.fault.signal.address}，无合法映射`, { red: true, width: 520 })}
    ${wires("unmapped-wires", [
      ["unmapped-query", "M155 210H145V430H200", "cyan"],
      ["unmapped-walk", "M700 470C825 470 790 310 880 310", "blue"],
    ])}</div>`,
    11,
    {
      stops: [3.6, 6.3, 9],
      checks: [
        { at: 8.7, selector: "#unmapped-AL .reg-value", text: "—" },
        { at: 8.7, selector: "#unmapped-mmu .translation", text: "07 → MISS" },
        { at: 6.5, selector: "#unmapped-mode", shown: true },
      ],
    },
  );
  ["#unmapped-mmu", "#unmapped-AL"].forEach((e) =>
    reveal(s.timeline, s.$(e), 0.1),
  );
  flow(s, "unmapped-query", 0.7, 0.8);
  reveal(s.timeline, s.$("#unmapped-mmu-tlb"), 1.7);
  reveal(s.timeline, s.$("#unmapped-PTE"), 2.2);
  flow(s, "unmapped-walk", 2.7, 1.1);
  reveal(s.timeline, s.$("#unmapped-mode"), 4.6);
  reveal(s.timeline, s.$("#unmapped-code"), 5.2);
  activeCode(s, 0, 5.5);
  activeCode(s, 1, 6.5);
  reveal(s.timeline, s.$("#unmapped-note"), 7.3);
  state(s, "unmapped-rejected-no-allocation", 6.5);
  return closeShot(s, { exit: false });
}
mountFilm(
  {
    id: "address-translation",
    chapter: 9,
    title: "虚拟地址如何找到数据？",
    actorContinuity: [actor('same-mmu-hardware','mmu',[
      ['vm-tlb-hit','#hit-mmu'],['vm-resident-miss','#miss-mmu'],['vm-legal-fault','#page-mmu'],['vm-write-denied','#denied-mmu'],['vm-unmapped','#unmapped-mmu']
    ])],
    model:
      "12 位 VA · 64B 页 · 单级教学页表 · 偏移 2D 保持 · TLB ≠ 程序数据 Cache",
    scenes: [
      { title: "拆开地址", detail: "页号与偏移", build: bitsIntro },
      { title: "TLB 命中", detail: "权限仍需检查", build: tlbHit },
      { title: "未命中但驻留", detail: "硬件继续查页表", build: residentMiss },
      { title: "合法缺页", detail: "补页 → 重试 → 回填", build: legalFault },
      { title: "只读保护", detail: "命中也禁止写", build: writeDenied },
      { title: "未映射地址", detail: "不能随意补页", build: unmapped },
    ],
  },
  "vm-access",
);
