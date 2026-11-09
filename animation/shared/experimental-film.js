import { mountFilm } from "./player.js";
import {
  shot,
  closeShot,
  pos,
  heading,
  code,
  register,
  note,
  wires,
  flow,
  reveal,
  number,
  COLORS,
} from "./scene-kit.js";
import {
  experiments,
  experimentalCues,
  experimentDurations,
  sourceExcerpt,
  cueCommands,
  recursionStackLimit,
} from "../models/experiments.js";

const scopes = {
  "buffer-copy":
    "这里的 input、buf、栈字节和返回位置来自实际运行；接下来用同样的真实写入记录逐格解释，越界行为只对这次编译成立。",
  "stack-recursion":
    "记录到实际递归寄存器、栈上限与信号；机制段选择部分帧作阅读压缩，正常返回来自另一次有终止条件的运行。",
  "cache-access":
    "实测能看到 C 的加载地址与字节，不能看到硬件 Cache 的全部 tag／LRU／HIT；后面的 Cache 使用相同读取序列作明确的机制模型。",
  "process-lifecycle":
    "fork 返回、PID、exec、输出和 wait 状态来自实际运行。launcher／worker 是本程序两种启动模式，exec 用 --worker 重新启动；CPU 调度仍是机制解释，不声称 GDB 证明了每一次调度。",
  "fork-memory":
    "真实观察同一虚拟地址和私有／共享值的变化；物理页、COW 复制与内核对象用模型解释，不能把 VA／maps 当作 PFN 实测。",
  "vm-access":
    "mmap 地址、读值、OS present 位和 SIGSEGV 是实测；TLB、PFN 与页表过程仍属模型。Linux 首次匿名读取可能使用共享零页，不能仅由 present 变化断言已分配一份私有物理页。",
};
const notes = {
  "buffer-copy":
    "真实 C／GDB 离线回放 · 当前编译的布局与越界结果，不是通用 C 保证",
  "stack-recursion": `本实验栈上限 ${recursionStackLimit / 1024}KiB · 机制段选择 8 个代表帧 · 正常返回为独立运行`,
  "cache-access":
    "C 地址／字节实测 · 4组×2路×8B Cache、HIT／LRU 为机制模型，不是硬件计数",
  "process-lifecycle":
    "真实 C／GDB PID、输出与 wait 状态 · CPU 调度示意 · 两次独立运行",
  "fork-memory":
    "C／GDB 虚拟地址与读值实测 · 物理页、COW 与内核结构示意，不是 PFN 实测",
  "vm-access":
    "C／GDB 访问、OS 驻留位与信号实测 · TLB／物理页号为模型 · 实机4KiB≠模型64B",
};

function buildExperiment(trace, cues, duration, stretch) {
  const id = `${trace.id}-experiment`;
  const authored = duration / stretch;
  const spacing = (duration - 2) / cues.length;
  const prefix = (i) => `${id}-step-${i}`;
  const markup = cues
    .map((cue, i) => {
      const p = prefix(i),
        excerpt = sourceExcerpt(trace, cue);
      const source = code(`${p}-source`, 90, 235, excerpt.lines, {
        width: 650,
        title: `${trace.provenance.source.path} / 源句节选（省略采集标记）`,
        lineNumbers: excerpt.numbers,
      }).replace(" data-anim", "");
      const fields = cue.readings
        .map(([label, , role], j) =>
          register(`${p}-field-${j}`, 860, 235 + j * 95, label, "—", {
            width: 440,
          })
            .replace(" data-anim", "")
            .replace(
              "register-cell",
              `register-cell measured-address experiment-reading ${role || ""}`,
            ),
        )
        .join("");
      return pos(
        p,
        0,
        0,
        `${source}${fields}
      ${pos(`${p}-case`, 90, 205, `独立运行：${cue.run} · 原始停点 #${cue.sampleIndex} · ${cue.title}`, "experiment-case", { visible: true })}
      ${pos(`${p}-commands`, 860, 555, `<div class="code-heading">GDB / 原样命令与输出节选</div><pre class="gdb-excerpt">${escapeText(cueCommands(trace, cue))}</pre>`, "experiment-gdb", { width: 440 })}
      ${note(`${p}-note`, 90, 650, cue.title, `GDB 停于源行 ${trace.runs[cue.run].samples[cue.sampleIndex].source.line || "机器指令"}；相关源句见左，完整程序与命令见下方实验台。`, { width: 650 })}
      ${wires(`${p}-wires`, [[`${p}-observe`, "M745 350C785 350 790 360 838 360", "cyan", "观测"]])}`,
        "experiment-step",
      );
    })
    .join("");
  const s = shot(
    id,
    `<div class="camera">${heading(`${id}-heading`, 90, 105, "REAL C PROGRAM / REAL GDB STOPS", "先运行 C，<em>再解释看见的结果</em>。")}${markup}</div>`,
    authored,
    {
      stops: cues.map((c) => c.time / stretch),
      checks: cues.flatMap((cue, i) =>
        cue.readings.map(([, value], j) => ({
          at: (cue.time + 0.45) / stretch,
          selector: `#${prefix(i)}-field-${j} .reg-value`,
          text: String(value),
        })),
      ),
    },
  );
  s.root.classList.add("experiment-scene");
  reveal(s.timeline, s.$(`#${id}-heading`), 0.1 / stretch, {
    duration: 0.4 / stretch,
    y: 0,
  });
  cues.forEach((cue, i) => {
    const p = prefix(i),
      start = (0.55 + i * spacing) / stretch;
    if (i)
      s.timeline.set(
        s.$(`#${prefix(i - 1)}`),
        { autoAlpha: 0 },
        start - 0.08 / stretch,
      );
    reveal(s.timeline, s.$(`#${p}`), start, { duration: 0.28 / stretch, y: 0 });
    for (const row of s.$$(`#${p} .code-line`))
      s.timeline.set(
        row,
        {
          color:
            Number(row.dataset.sourceLine) === cue.focusLine
              ? COLORS.cyan
              : COLORS.ink,
        },
        start,
      );
    flow(s, `${p}-observe`, start + 0.3 / stretch, 0.7 / stretch);
    cue.readings.forEach(([, value], j) =>
      number(
        s.timeline,
        s.$(`#${p}-field-${j} .reg-value`),
        String(value),
        cue.time / stretch,
      ),
    );
    reveal(s.timeline, s.$(`#${p}-commands`), (cue.time + 0.05) / stretch, {
      duration: 0.3 / stretch,
      y: 0,
    });
    reveal(s.timeline, s.$(`#${p}-note`), (cue.time + 0.15) / stretch, {
      duration: 0.3 / stretch,
      y: 0,
    });
  });
  return closeShot(s);
}
function escapeText(value) {
  return String(value).replace(
    /[&<>]/g,
    (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;" })[c],
  );
}
export function mountExperimentalFilm(config, id) {
  const trace = experiments[id],
    duration = experimentDurations[id],
    stretch = config.stretch || 1;
  const spacing = (duration - 2) / experimentalCues(id).length;
  const cues = experimentalCues(id).map((cue, i) => ({
    ...cue,
    time: Math.round((0.55 + i * spacing + 1.08) * 1000) / 1000,
  }));
  const samples = Object.values(trace.runs).reduce(
    (sum, run) => sum + run.samples.length,
    0,
  );
  const kind = id === "buffer-copy" ? "gdb-measured" : "gdb-plus-model";
  mountFilm({
    ...config,
    model: notes[id],
    example: `../${trace.provenance.source.path}`,
    provenance: {
      ...trace.provenance,
      kind,
      samples,
      events: samples,
      sourceHref: `../${trace.provenance.source.path}`,
      jsonHref: `../traces/${id}.json`,
      gdbHref: `../traces/${id}-gdb.txt`,
    },
    experiment: {
      trace,
      cues,
      duration,
      scope: scopes[id],
      legacyExample: config.example || null,
    },
    steps: config.steps
      ? [
          ...cues.map((c) => c.time / stretch),
          ...config.steps.map((t) => t + duration / stretch),
        ]
      : undefined,
    scenes: [
      {
        title: "C 程序与 GDB",
        detail: "真实程序／实际停点",
        sourceKind: "gdb-measured",
        view: "C + GDB OBSERVATION",
        build: () => buildExperiment(trace, cues, duration, stretch),
      },
      ...config.scenes.map((scene) => ({
        ...scene,
        sourceKind: kind === "gdb-measured" ? "gdb-measured" : "teaching-model",
      })),
    ],
  });
}
