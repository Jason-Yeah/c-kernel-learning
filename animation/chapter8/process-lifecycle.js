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
  windowBox,
  wires,
  flow,
  reveal,
  vanish,
  state,
} from "../shared/scene-kit.js";
import { lifecycleCases, processTrace } from "../models/computer.js";
import { experiments, observedProcesses } from "../models/experiments.js";
const E = observedProcesses.early,
  L = observedProcesses.late;
const traces = lifecycleCases.map((events, i) =>
  processTrace(events, i === 0 ? E : L),
);
const program =
  experiments["process-lifecycle"].provenance.source.content.split("\n");
function realCode(id, x, y, terms, options = {}, from = 0) {
  const indices = terms.map((term) =>
    program.findIndex((line, i) => i >= from && line.includes(term)),
  );
  if (indices.some((i) => i < 0)) throw new Error("C source fragment missing");
  return code(
    id,
    x,
    y,
    indices.map((i) => program[i].replace(/\s*\/\*.*?\*\//g, "").trim()),
    {
      ...options,
      title: "process-lifecycle.c / 真实源码节选",
      lineNumbers: indices.map((i) => i + 1),
    },
  );
}
const processWindow = (id, x, y, pid, image, status, visible = false) =>
  windowBox(
    id,
    x,
    y,
    `PROCESS / PID ${pid}`,
    `<div class="process-pid">${pid}</div><div class="process-state">${status}</div><div class="process-image">${image}</div>`,
    { width: 335, visible },
  );
function intro() {
  const s = shot(
    "process-intro",
    `<div class="camera">${processWindow("intro-parent", 550, 250, E.parent, "launcher", "运行中", true)}
    ${realCode("intro-fork-code", 170, 460, ["fork_result = fork();", "if (fork_result == 0)"], { width: 610 })}
    ${heading("intro-process-title", 170, 130, "ONE PROGRAM / TWO CONTEXTS", "从一个进程<em>开始</em>。")}
    ${note("intro-fork-note", 935, 535, "fork：建立子进程", "不是在这里立即复制整份 RAM。", { width: 320 })}</div>`,
    8,
    {
      stops: [3.5, 6.8],
      checks: [
        {
          at: 2,
          selector: "#intro-parent .process-pid",
          text: String(E.parent),
        },
      ],
    },
  );
  reveal(s.timeline, s.$("#intro-process-title"), 0.2);
  s.timeline.to(
    s.$("#intro-parent"),
    { x: 375, y: -5, duration: 1.1, ease: "power2.inOut" },
    0.7,
  );
  reveal(s.timeline, s.$("#intro-fork-code"), 1.5);
  activeCode(s, 0, 2.3);
  reveal(s.timeline, s.$("#intro-fork-note"), 4);
  state(s, "before-fork", 0);
  return closeShot(s);
}
function forkReturns() {
  const s = shot(
    "process-fork-returns",
    `<div class="camera">${processWindow("fork-parent", 170, 180, E.parent, "launcher", "运行", true)}${processWindow("fork-child", 935, 180, E.child, "launcher", "就绪")}
    ${chip("fork-cpu", 600, 340, { width: 240 })}${register("fork-parent-return", 170, 520, "父：fork 返回值", "—", { width: 335 })}${register("fork-child-return", 935, 520, "子：fork 返回值", "—", { width: 335 })}
    ${pos("fork-kernel", 620, 250, "内核态 / 创建执行上下文", "mode-badge")}${note("fork-return-note", 505, 650, "一次调用，两份返回上下文", "谁先运行由调度决定；这里仍只有一颗 CPU。", { width: 670 })}
    ${wires("fork-wires", [
      ["fork-create", "M830 418C885 418 848 290 920 290", "blue"],
      ["fork-p-return", "M590 430C550 430 565 570 520 570", "cyan", "PID"],
      ["fork-c-return", "M835 430C892 430 858 570 920 570", "cyan", "0"],
    ])}</div>`,
    10,
    {
      stops: [3.9, 6.9, 8.5],
      checks: [
        { at: 1, selector: "#fork-child", shown: false },
        {
          at: 7,
          selector: "#fork-parent-return .reg-value",
          text: String(E.child),
        },
        { at: 7, selector: "#fork-child-return .reg-value", text: "0" },
      ],
    },
  );
  reveal(s.timeline, s.$("#fork-cpu"), 1.15);
  reveal(s.timeline, s.$("#fork-kernel"), 1);
  flow(s, "fork-create", 1.4, 1.3);
  reveal(s.timeline, s.$("#fork-child"), 2.7, { x: -30, y: 0 });
  reveal(s.timeline, s.$("#fork-parent-return"), 3);
  reveal(s.timeline, s.$("#fork-child-return"), 3.2);
  vanish(s.timeline, s.$("#fork-kernel"), 4);
  flow(s, "fork-p-return", 4.3, 1.1);
  flow(s, "fork-c-return", 4.3, 1.1);
  number(s.timeline, s.$("#fork-parent-return .reg-value"), E.child, 5.5);
  number(s.timeline, s.$("#fork-child-return .reg-value"), 0, 5.5);
  reveal(s.timeline, s.$("#fork-return-note"), 6.1);
  state(s, "fork-contexts-ready", 5.5);
  return closeShot(s);
}
function earlyWait() {
  const output = traces[0].at(-1).output;
  const s = shot(
    "process-early-wait",
    `<div class="camera">${processWindow("early-parent", 150, 140, E.parent, "launcher", "运行", true)}${processWindow("early-child", 960, 140, E.child, "launcher", "就绪", true)}
    ${chip("early-cpu", 600, 365, { width: 240, subtitle: "一个执行令牌" })}${register("early-owner", 620, 200, "当前 CPU 属于", String(E.parent), { width: 200 })}
    ${pos("early-stdout", 180, 665, " ", "terminal")}${realCode("early-code", 125, 402, ["send_byte(STDOUT_FILENO, 'P');", "wait_result = waitpid("], { width: 440 })}
    ${note("early-exec-note", 940, 600, "exec：换映像，不换 PID", "worker 从新入口开始，不回旧调用点。", { width: 335 })}
    ${wires("early-wires", [
      [
        "early-P",
        "M310 395H65V710H165",
        "cyan",
        "P",
      ],
      ["early-switch", "M810 510C890 510 870 300 945 300", "blue"],
      ["early-C", "M1100 360C1100 650 560 710 430 710", "cyan", "C"],
    ])}</div>`,
    10,
    {
      stops: [2.1, 4.8, 7.4, 9.1],
      checks: [
        {
          at: 4.6,
          selector: "#early-parent .process-state",
          text: "等待 · 不占 CPU",
        },
        { at: 4.6, selector: "#early-owner .reg-value", text: String(E.child) },
        { at: 7.2, selector: "#early-child .process-image", text: "worker" },
        { at: 9.3, selector: "#early-stdout", text: output },
      ],
    },
  );
  reveal(s.timeline, s.$("#early-cpu"), 0.1);
  reveal(s.timeline, s.$("#early-owner"), 0.2);
  reveal(s.timeline, s.$("#early-code"), 0.3);
  reveal(s.timeline, s.$("#early-stdout"), 0.4);
  activeCode(s, 0, 0.5);
  flow(s, "early-P", 0.6, 1);
  number(s.timeline, s.$("#early-stdout"), "P", 1.7);
  activeCode(s, 1, 2.2);
  number(
    s.timeline,
    s.$("#early-parent .process-state"),
    "等待 · 不占 CPU",
    3.3,
  );
  flow(s, "early-switch", 3.3, 0.9);
  number(s.timeline, s.$("#early-owner .reg-value"), E.child, 4.3);
  number(s.timeline, s.$("#early-child .process-state"), "运行", 4.3);
  s.timeline.to(
    s.$("#early-child .process-image"),
    { opacity: 0.08, x: -12, duration: 0.45 },
    5.1,
  );
  number(s.timeline, s.$("#early-child .process-image"), "worker", 5.6);
  s.timeline.to(
    s.$("#early-child .process-image"),
    { opacity: 1, x: 0, duration: 0.65 },
    5.7,
  );
  reveal(s.timeline, s.$("#early-exec-note"), 6.3);
  flow(s, "early-C", 7.2, 1.1);
  number(s.timeline, s.$("#early-stdout"), output, 8.4);
  state(s, "parent-wait-child-worker", 6);
  return closeShot(s);
}
function earlyReap() {
  const final = traces[0].at(-1);
  const s = shot(
    "process-exit-reap",
    `<div class="camera">${processWindow("reap-parent", 160, 160, E.parent, "launcher", "等待", true)}${processWindow("reap-child", 940, 160, E.child, "worker", "运行", true)}
    ${pos("reap-zombie", 940, 495, `PID <b>${E.child}</b> · Z<br>exit code = <b>7</b><br>仅退出记录，已无用户映像`, "task-record")}
    ${register("reap-return", 160, 480, "waitpid 返回值", "—", { width: 300 })}${register("reap-status", 500, 480, "原始 status", "—", { width: 280 })}
    ${pos("reap-output",940,650,"PC","terminal")}${pos("reap-kernel", 590, 300, "同一 CPU / 内核态", "mode-badge")}${note("reap-note", 160, 650, "wait 返回 PID，不是退出码", "WEXITSTATUS(0x0700) = 7", { width: 600 })}
    ${wires("reap-wires", [
      ["reap-exit", "M1115 390C1115 430 1090 445 1090 475", "blue", "7"],
      ["reap-wake", "M925 550C790 550 820 330 510 330", "cyan"],
    ])}</div>`,
    8,
    {
      stops: [3.3, 5.9, 7.1],
      checks: [
        { at: 3.7, selector: "#reap-zombie", shown: true },
        { at: 6.8, selector: "#reap-zombie", shown: false },
        {
          at: 6.9,
          selector: "#reap-return .reg-value",
          text: String(final.wait),
        },
        { at: 6.9, selector: "#reap-status .reg-value", text: "0x0700" },
      ],
    },
  );
  ["#reap-return", "#reap-status"].forEach((e) =>
    reveal(s.timeline, s.$(e), 0.1),
  );
  reveal(s.timeline, s.$("#reap-output"), 0.2);
  reveal(s.timeline, s.$("#reap-kernel"), 0.7);
  flow(s, "reap-exit", 1, 1);
  vanish(s.timeline, s.$("#reap-child .process-image"), 1.6, { y: -15 });
  number(
    s.timeline,
    s.$("#reap-child .process-state"),
    "已退出 · 用户映像已释放",
    2.2,
  );
  reveal(s.timeline, s.$("#reap-zombie"), 2.4);
  flow(s, "reap-wake", 3.3, 1);
  number(s.timeline, s.$("#reap-parent .process-state"), "唤醒 → 运行", 4.5);
  number(s.timeline, s.$("#reap-return .reg-value"), final.wait, 5.1);
  number(s.timeline, s.$("#reap-status .reg-value"), "0x0700", 5.4);
  vanish(s.timeline, s.$("#reap-zombie"), 5.8);
  vanish(s.timeline, s.$("#reap-kernel"), 5.8);
  reveal(s.timeline, s.$("#reap-note"), 6);
  state(s, "child-reaped", 6.3);
  return closeShot(s);
}
function childFirst() {
  const s = shot(
    "process-child-first",
    `<div class="camera">${heading("late-reset", 155, 110, "ANOTHER LEGAL SCHEDULE", "换一种调度，<em>先运行子进程</em>。")}
    ${processWindow("late-parent", 170, 295, L.parent, "launcher", "就绪")}${processWindow("late-child", 920, 295, L.child, "launcher", "运行 · fork 返回 0")}
    ${pos("late-token", 600, 330, `<div class="eyebrow">CPU OWNER</div><div class="big-result">${L.child}</div>`, "")}${pos("late-stdout", 570, 620, " ", "terminal")}
    ${realCode("late-exec", 145, 540, ["execl("], { width: 410 })}
    ${wires("late-wires", [["late-C", "M1090 540C1090 650 920 662 790 662", "cyan", "C"]])}</div>`,
    8,
    {
      stops: [3.8, 6.1, 7],
      checks: [
        { at: 4.5, selector: "#late-child .process-image", text: "worker" },
        { at: 6.4, selector: "#late-stdout", text: "C" },
      ],
    },
  );
  reveal(s.timeline, s.$("#late-reset"), 0.1);
  reveal(s.timeline, s.$("#late-parent"), 0.4, { x: -20, y: 0 });
  reveal(s.timeline, s.$("#late-child"), 0.6, { x: 20, y: 0 });
  reveal(s.timeline, s.$("#late-token"), 1);
  reveal(s.timeline, s.$("#late-exec"), 1.4);
  activeCode(s, 0, 1.8);
  s.timeline.to(
    s.$("#late-child .process-image"),
    { opacity: 0.1, duration: 0.4 },
    2.4,
  );
  number(s.timeline, s.$("#late-child .process-image"), "worker", 2.9);
  s.timeline.to(
    s.$("#late-child .process-image"),
    { opacity: 1, duration: 0.5 },
    3,
  );
  reveal(s.timeline, s.$("#late-stdout"), 3.9);
  flow(s, "late-C", 4.3, 1.1);
  number(s.timeline, s.$("#late-stdout"), "C", 5.5);
  number(s.timeline, s.$("#late-child .process-state"), "_exit(7) → 退出", 6.1);
  vanish(s.timeline, s.$("#late-child .process-image"), 6.2);
  state(s, "child-exited-parent-not-waited", 6.8);
  return closeShot(s);
}
function delayedReap() {
  const output = traces[1].at(-1).output;
  const s = shot(
    "process-delayed-reap",
    `<div class="camera">${processWindow("delay-parent", 170, 145, L.parent, "launcher", "运行 · 继续工作", true)}${pos("delay-zombie", 930, 205, `PID <b>${L.child}</b> · Z<br>用户映像：已释放<br>CPU：不占用<br>等待父进程回收`, "task-record", { visible: true })}
    ${chip("delay-cpu", 570, 340, { subtitle: `执行父进程 ${L.parent}`, width: 240 })}${pos("delay-output", 190, 665, "C", "terminal")}${register("delay-wait-value", 930, 530, "waitpid 返回", "—", { width: 310 })}
    ${note("delay-note", 930, 390, "退出，不等于回收", "父继续工作时，Z 记录仍然存在。", { width: 360 })}
    ${realCode(
      "delay-code",
      95,
      415,
      [
        "checkpoint(5);",
        "send_byte(STDOUT_FILENO, 'P');",
        "wait_result = waitpid(",
      ],
      { width: 460 },
      program.findIndex((line) => line.includes("checkpoint(5);")),
    )}
    ${wires("delay-wires", [["delay-P", "M345 382C140 382 65 385 65 430L65 595Q65 650 290 650", "cyan", "P"]])}</div>`,
    10,
    {
      stops: [3.3, 6.5, 9],
      checks: [
        { at: 6.5, selector: "#delay-zombie", shown: true },
        { at: 9.2, selector: "#delay-zombie", shown: false },
        {
          at: 9.2,
          selector: "#delay-wait-value .reg-value",
          text: String(L.child),
        },
        { at: 3.4, selector: "#delay-output", text: output },
      ],
    },
  );
  reveal(s.timeline, s.$('#delay-code'),0.65,{y:0,duration:0.3});
  ["#delay-cpu", "#delay-output", "#delay-wait-value"].forEach(
    (e) => reveal(s.timeline, s.$(e), 0.2),
  );
  activeCode(s, 0, 0.6);
  flow(s, "delay-P", 0.8, 1.1);
  number(s.timeline, s.$("#delay-output"), output, 2);
  reveal(s.timeline, s.$("#delay-note"), 2.3);
  activeCode(s, 1, 3);
  activeCode(s, 2, 7.1);
  number(s.timeline, s.$("#delay-wait-value .reg-value"), L.child, 8.1);
  vanish(s.timeline, s.$("#delay-zombie"), 8.3);
  state(s, "delayed-reap-complete", 8.8);
  return closeShot(s);
}
function summary() {
  const s = shot(
    "process-summary",
    `<div class="camera">${heading("process-summary-title", 170, 130, "ORDER IS NOT GUARANTEED", "调度决定先后，<em>回收结束生命周期</em>。")}
    ${windowBox("summary-PC", 190, 300, "父先运行 / 提前等待", '<div class="big-result">PC</div><small>先写 P，再等子进程写 C</small>', { width: 430 })}${windowBox("summary-CP", 810, 300, "子先运行 / 延迟回收", '<div class="big-result">CP</div><small>子先写 C，父稍后写 P 并回收</small>', { width: 430 })}
    ${realCode("process-summary-code", 190, 565, ["wait_result = waitpid(", "return wait_result == child_id"], { width: 840 })}
    ${note("process-summary-note", 1030, 585, "exec ≠ fork", "换映像，不换 PID。", { width: 260 })}</div>`,
    10,
    {
      stops: [4.5, 8.5],
      checks: [
        { at: 7, selector: "#summary-PC .big-result", text: "PC" },
        { at: 7, selector: "#summary-CP .big-result", text: "CP" },
      ],
    },
  );
  reveal(s.timeline, s.$("#process-summary-title"), 0.1);
  reveal(s.timeline, s.$("#summary-PC"), 0.8, { x: -30, y: 0 });
  reveal(s.timeline, s.$("#summary-CP"), 2.2, { x: 30, y: 0 });
  reveal(s.timeline, s.$("#process-summary-code"), 4.2);
  activeCode(s, 1, 5.2);
  reveal(s.timeline, s.$("#process-summary-note"), 6.1);
  return closeShot(s, { exit: false });
}
mountFilm(
  {
    id: "process-lifecycle",
    chapter: 8,
    title: "fork、exec 与进程生命周期",
    actorContinuity: [
      actor('early-parent','process',[['process-intro','#intro-parent'],['process-fork-returns','#fork-parent'],['process-early-wait','#early-parent'],['process-exit-reap','#reap-parent']]),
      actor('early-child','process',[['process-fork-returns','#fork-child'],['process-early-wait','#early-child'],['process-exit-reap','#reap-child']]),
      actor('early-cpu','chip',[['process-fork-returns','#fork-cpu'],['process-early-wait','#early-cpu']]),
      actor('early-output','text',[['process-early-wait','#early-stdout'],['process-exit-reap','#reap-output']]),
      actor('late-parent','process',[['process-child-first','#late-parent'],['process-delayed-reap','#delay-parent',{move:0.6}]]),
      actor('late-output','text',[['process-child-first','#late-stdout'],['process-delayed-reap','#delay-output']]),
    ],
    model: "Linux · 单 CPU / 单线程 · 固定成功路径 · 两种合法调度，不穷举交错",
    scenes: [
      { title: "一个进程", detail: "fork 之前", build: intro },
      {
        title: "两份上下文",
        detail: `父 ${E.child} / 子 0`,
        build: forkReturns,
      },
      { title: "提前等待", detail: "父睡眠，子执行", build: earlyWait },
      { title: "退出与回收", detail: "PID 不等于退出码", build: earlyReap },
      { title: "另一种调度", detail: "子先运行", build: childFirst },
      { title: "延迟回收", detail: "Z 不占 CPU", build: delayedReap },
      { title: "对照与收束", detail: "PC / CP", build: summary },
    ],
  },
  "process-lifecycle",
);
