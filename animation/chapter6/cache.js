import { mountExperimentalFilm as mountFilm } from "../shared/experimental-film.js";
import { actor } from "../shared/actor-continuity.js";
import {
  shot,
  closeShot,
  pos,
  heading,
  note,
  code,
  register,
  number,
  page,
  ribbon,
  wires,
  flow,
  reveal,
  vanish,
  state,
  hex,
} from "../shared/scene-kit.js";
import { cacheRecords, splitAddress } from "../models/computer.js";
import { cacheReady } from "../models/experiments.js";
function cacheWay(id, snapshot, set, way, x, y, animated = false) {
  const line = snapshot[set][way],
    both = snapshot[set].every((v) => v.valid);
  const lru = both
    ? snapshot[set][0].used < snapshot[set][1].used
      ? 0
      : 1
    : -1;
  return pos(
    id,
    x,
    y,
    `<div class="cache-meta"><span>WAY ${way} · V=${Number(line.valid)}</span><b>TAG ${line.valid ? line.tag.toString(2).padStart(3, "0") : "---"}</b></div><div class="cache-bytes">${Array.from({ length: 8 }, (_, i) => `<span class="cache-byte" data-offset="${i}">${line.valid ? hex(line.data[i]) : "—"}</span>`).join("")}</div><div class="cache-rank">${line.valid ? `BLOCK ${hex(line.block)} · ${both ? (lru === way ? "LRU / 较久未用" : "MRU / 最近使用") : "另一条路尚空"}` : "EMPTY / 空路"}</div>`,
    "cache-way",
    { visible: !animated },
  );
}
function addressIntro() {
  const split = splitAddress(0);
  const s = shot(
    "cache-address",
    `<div class="camera">${pos("cache-first", 545, 260, '<div class="function-code" style="font-size:60px">value = *target</div><div class="eyebrow" style="margin-top:25px">ONE BYTE READ</div>', "", { visible: true })}
    ${pos("cache-bits", 365, 350, `<div class="bit-fields"><div class="bit-field"><b>${split.tag.toString(2).padStart(3, "0")}</b><small>TAG / 3 bit</small></div><div class="bit-field"><b>00</b><small>SET / 2 bit</small></div><div class="bit-field offset"><b>000</b><small>OFFSET / 3 bit</small></div></div>`, "")}
    ${note("cache-size-note", 365, 550, "4 组 × 2 路 × 每块 8 字节", `真实数组基址 ${cacheReady.base}；这里把同一读取偏移送入 8 位物理地址模型。`, { width: 800 })}</div>`,
    6,
    {
      stops: [3.9, 5.1],
      checks: [{ at: 4.4, selector: "#cache-bits", shown: true }],
    },
  );
  s.timeline.to(
    s.$("#cache-first"),
    { x: -180, y: -150, scale: 0.7, transformOrigin: "0 0", duration: 1.1 },
    0.4,
  );
  reveal(s.timeline, s.$("#cache-bits"), 1.5, { y: 0, scale: 0.95 });
  reveal(s.timeline, s.$("#cache-size-note"), 3.4);
  return closeShot(s);
}
const accessDurations = [10, 6, 8, 6, 9, 9, 8, 6];
const accessTitles = [
  "冷启动：把整块搬进来",
  "同一块，换一个偏移",
  "同一组，还有一条空路",
  "命中，并更新使用顺序",
  "组已满，替换最久未用",
  "再访被逐出的块",
  "换到第 1 组",
  "块内偏移，选中那个字节",
];
function access(record) {
  const r = record,
    n = r.number,
    duration = accessDurations[n],
    prefix = `cache-${n}`,
    hit = r.hit;
  const x = hit ? 495 : 190,
    ys = [285, 470];
  const commit = hit ? 2.6 : duration - 2.4,
    returned = duration - 1.2;
  const selectedByteX = x + 16 + r.offset * 44,
    selectedByteY = ys[r.way] + 54;
  const before = [0, 1]
    .map((way) =>
      cacheWay(`${prefix}-before-${way}`, r.before, r.index, way, x, ys[way]),
    )
    .join("");
  const after = [0, 1]
    .map((way) =>
      cacheWay(
        `${prefix}-after-${way}`,
        r.after,
        r.index,
        way,
        x,
        ys[way],
        true,
      ),
    )
    .join("");
  const routes = [
    [
      `${prefix}-byte-out`,
      `M${selectedByteX + 20} ${ys[r.way] + 94}H${x + 414}C${hit ? 960 : 790} ${ys[r.way] + 94} 940 ${hit ? 410 : 663} 1050 ${hit ? 410 : 663}`,
      "cyan",
    ],
  ];
  if (!hit)
    routes.push([
      `${prefix}-block-in`,
      `M970 400C800 400 840 ${ys[r.way] + 75} ${x + 415} ${ys[r.way] + 75}`,
      "blue",
      "8B",
    ]);
  if (!hit && !r.empty)
    routes.push([
      `${prefix}-evict`,
      `M${x + 415} ${ys[r.way] + 135}H715V680`,
      "blue",
    ]);
  const decision = hit
    ? `HIT：组 ${r.index} / 路 ${r.way}`
    : r.empty
      ? `MISS：选择空路 ${r.way}`
      : `MISS：淘汰块 ${hex(r.victim.block)}`;
  const why = hit
    ? "这次不用访问 RAM，直接选取块内字节。"
    : r.empty
      ? n === 6 ? "只换焦点，组0未清空；本组先装整块。" : "先搬入完整 8B，再从中选一个字节。"
      : n === 4
        ? "首次访问新块 40；替换不是程序异常。"
        : "块 20 被同组竞争逐出，这次需要重新装入。";
  const s = shot(
    `cache-access-${n}`,
    `<div class="camera">
    ${heading(`${prefix}-title`, 170, 105, `ACCESS ${n + 1}/08 · MODEL PA 0x${hex(r.address)}`, accessTitles[n])}
    ${register(`${prefix}-counts`, 1120, 115, "HIT / MISS", `${r.previous.hits} / ${r.previous.misses}`, { width: 190 })}
    ${pos(`${prefix}-set`, x, 236, `<div class="mini-group">${[0, 1, 2, 3].map((i) => `<span style="${i === r.index ? "color:#bdeefa;border-color:#69e2f4" : ""}">组 ${i}</span>`).join("")}</div>`, "")}
    ${before}${after}${pos(`${prefix}-highlight`, selectedByteX, selectedByteY, "", "cache-highlight")}
    ${register(`${prefix}-AL`, 1070, hit ? 355 : 610, "CPU / AL", r.previous.al === null ? "—" : hex(r.previous.al), { width: 230 })}
    ${hit ? code(`${prefix}-code`, 155, 320, [`// offset = 0x${hex(r.address)}`, "value = *target;"], { width: 305, title: "C 实际读取句 / 模型偏移" }) : page(`${prefix}-ram`, 990, 310, `RAM / BLOCK ${hex(r.block)}`, "8B")}
    ${
      hit
        ? ""
        : ribbon(
            `${prefix}-ram-bytes`,
            965,
            535,
            r.after[r.index][r.way].data.map((v) => hex(v)),
            { cellWidth: 30, labels: false },
          )
    }
    ${note(`${prefix}-decision`, hit ? 160 : 190, hit ? 535 : 674, decision, why, { width: hit ? 310 : 690 })}
    ${pos(`${prefix}-bits`, hit ? 155 : 670, hit ? 250 : 260, `TAG ${r.tag.toString(2).padStart(3, "0")} / SET ${r.index.toString(2).padStart(2, "0")} / OFF ${r.offset.toString(2).padStart(3, "0")}`, "tiny-note mono")}
    ${wires(`${prefix}-wires`, routes)}</div>`,
    duration,
    {
      stops: [Math.min(3.1, duration - 2), commit + 0.4, returned + 0.2],
      checks: [
        {
          at: commit - 0.15,
          selector: `#${prefix}-before-${r.way} .cache-meta b`,
          text: `TAG ${r.before[r.index][r.way].valid ? r.before[r.index][r.way].tag.toString(2).padStart(3,'0') : '---'}`,
        },
        {
          at: returned - 0.2,
          selector: `#${prefix}-AL .reg-value`,
          text: r.previous.al === null ? "—" : hex(r.previous.al),
        },
        {
          at: returned + 0.35,
          selector: `#${prefix}-AL .reg-value`,
          text: hex(r.value),
        },
        {
          at: commit + 0.4,
          selector: `#${prefix}-after-${r.way} .cache-meta b`,
          text: `TAG ${r.tag.toString(2).padStart(3, "0")}`,
        },
      ],
    },
  );
  reveal(s.timeline, s.$(`#${prefix}-title`), 0.05);
  reveal(s.timeline, s.$(`#${prefix}-set`), 0.3);
  reveal(s.timeline, s.$(`#${prefix}-counts`), 0.15);
  reveal(s.timeline, s.$(`#${prefix}-AL`), 0.2);
  reveal(s.timeline, s.$(`#${prefix}-bits`), 0.65);
  reveal(s.timeline, s.$(`#${prefix}-decision`), 1.7);
  number(
    s.timeline,
    s.$(`#${prefix}-counts .reg-value`),
    `${r.hits} / ${r.misses}`,
    2.2,
  );
  if (hit) reveal(s.timeline, s.$(`#${prefix}-code`), 1.15);
  else {
    reveal(s.timeline, s.$(`#${prefix}-ram`), 2.3);
    reveal(s.timeline, s.$(`#${prefix}-ram-bytes`), 2.8);
    if (!r.empty) {
      flow(s, `${prefix}-evict`, commit - 2.2, 0.9);
      s.timeline.to(
        s.$(`#${prefix}-before-${r.way} .cache-bytes`),
        { opacity: 0.15, x: -20, duration: 0.8 },
        commit - 1.1,
      );
    }
    flow(s, `${prefix}-block-in`, commit - 1.18, 1.1);
  }
  for (const way of [0, 1]) {
    // The capacity frame survives. Only its fields/data change at commit.
    s.timeline.set(s.$(`#${prefix}-after-${way} .cache-bytes`), {opacity:1,x:0}, commit);
  }
  state(s, "cache-state-committed", commit);
  reveal(s.timeline, s.$(`#${prefix}-highlight`), commit + 0.22, {
    y: 0,
    duration: 0.2,
  });
  flow(s, `${prefix}-byte-out`, returned - 1.08, 1);
  number(s.timeline, s.$(`#${prefix}-AL .reg-value`), hex(r.value), returned);
  state(s, "byte-returned", returned);
  return closeShot(s);
}
function summary() {
  const s = shot(
    "cache-summary",
    `<div class="camera">${heading("cache-summary-title", 180, 120, "EIGHT READS / ONE HISTORY", "命中，来自<em>已经留下的块</em>。")}
    ${pos("cache-summary-sequence", 180, 335, cacheRecords.map((r, i) => `<div id="cache-result-${i}" class="sequence-cell" data-anim><code>${hex(r.address)}</code><strong>${r.hit ? "HIT" : "MISS"}</strong></div>`).join(""), "result-sequence")}
    ${pos("cache-hit-rate", 180, 550, '<div class="big-result">37.5%</div><div class="soft-label">3 HIT / 5 MISS</div>')}
    ${note("cache-summary-note", 690, 565, "局部复用与同组竞争", "本片是固定参数示例，不模拟地址翻译或写策略。", { width: 590 })}</div>`,
    4,
    {
      stops: [2.5, 3.5],
      checks: [{ at: 3.3, selector: "#cache-hit-rate", shown: true }],
    },
  );
  reveal(s.timeline, s.$("#cache-summary-title"), 0.05);
  reveal(s.timeline, s.$("#cache-summary-sequence"), 0.1, { y: 0 });
  cacheRecords.forEach((_, i) =>
    reveal(s.timeline, s.$(`#cache-result-${i}`), 0.3 + i * 0.16, {
      y: 10,
      duration: 0.3,
    }),
  );
  reveal(s.timeline, s.$("#cache-hit-rate"), 1.9);
  reveal(s.timeline, s.$("#cache-summary-note"), 2.1);
  return closeShot(s, { exit: false });
}
mountFilm(
  {
    id: "cache",
    chapter: 6,
    title: "Cache：从地址到那个字节",
    actorContinuity: [
      actor('cpu-AL','register',cacheRecords.map(r=>[`cache-access-${r.number}`,`#cache-${r.number}-AL`])),
      actor('cache-hit-miss-counts','register',cacheRecords.map(r=>[`cache-access-${r.number}`,`#cache-${r.number}-counts`])),
      ...[[0,[0,1,2,3,4,5]],[1,[6,7]]].flatMap(([set,indices])=>[0,1].map(way=>actor(`cache-set-${set}-way-${way}`,'cache',indices.flatMap(i=>[
        [`cache-access-${i}`,`#cache-${i}-before-${way}`],
        [`cache-access-${i}`,`#cache-${i}-after-${way}`,{at:cacheRecords[i].hit?2.6:accessDurations[i]-2.4,move:0}]
      ])))),
    ],
    model: "4 组 × 2 路 × 8B · 8 位物理地址 · LRU 教学模型 · Cache miss ≠ 缺页",
    scenes: [
      { title: "地址拆分", detail: "TAG / SET / OFFSET", build: addressIntro },
      ...cacheRecords.map((r) => ({
        title: `${hex(r.address)} · ${r.hit ? "命中" : "未命中"}`,
        detail: accessTitles[r.number],
        build: () => access(r),
      })),
      { title: "八次访问", detail: "3 HIT / 5 MISS", build: summary },
    ],
  },
  "cache-access",
);
