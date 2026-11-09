import { gsap, escape as escapeHTML, setActorContext } from "./scene-kit.js";
import { prepareActorContinuity } from "./actor-continuity.js";
import {
  experimentWorkbench,
  bindExperimentWorkbench,
} from "./experiment-view.js";

function evidencePanel(p, includeSource = false) {
  if (!p) return "";
  const kind =
    p.kind === "gdb-plus-model" ? "C／GDB 实测 + 机制模型" : "GDB 实测";
  const source =
    includeSource && p.source?.content
      ? `<details class="embedded-program"><summary>展开完整 C 程序</summary><pre>${escapeHTML(p.source.content)}</pre></details>`
      : "";
  return `<details class="provenance" id="provenance"><summary>数据来源：${kind} · ${p.samples} 个实际停点 / ${p.events} 个事件（离线回放）</summary><div class="provenance-body"><p>地址、寄存器、变量和指令来自同一次采集。箭头、构图和教学速度是解释性设计，不代表 CPU 耗时，也不是浏览器在实时运行 GDB。</p><div class="evidence-links"><a href="${p.sourceHref}">实验源码 ↗</a><a href="${p.jsonHref}">原始采集 JSON ↗</a><a href="${p.gdbHref}">GDB 寄存器／栈文本 ↗</a></div><dl><dt>采集时间</dt><dd>${escapeHTML(p.capturedAt)}</dd><dt>工具</dt><dd>${escapeHTML(p.compiler)} · ${escapeHTML(p.gdb)} · ${escapeHTML(p.architecture)}</dd><dt>编译命令</dt><dd><code>${escapeHTML(p.compilerCommand)}</code></dd><dt>ASLR</dt><dd>系统 randomize_va_space=${escapeHTML(p.kernelASLR)}；${escapeHTML(p.debuggerASLR)} 本次未修改系统配置。</dd><dt>源码 SHA-256</dt><dd><code>${escapeHTML(p.source.sha256)}</code></dd><dt>ELF SHA-256</dt><dd><code>${escapeHTML(p.binary.sha256)}</code></dd></dl>${source}<p>特别注意：变量在作用域中不等于已经初始化。函数入口参数先核对寄存器；局部变量仅在初始化完成后用于动画。重采时地址可能变化，不推广为所有编译器的固定布局。</p></div></details>`;
}

const icon = (path, cls = "") =>
  `<svg class="${cls}" viewBox="0 0 24 24" aria-hidden="true"><path d="${path}"/></svg>`;
const playIcon = icon("m8 5 11 7-11 7z", "play-icon");
const pauseIcon = icon("M7 5h4v14H7zM14 5h4v14h-4z", "pause-icon");

export function mountFilm(config) {
  const { chapter, title, id, model, scenes, stretch = 1 } = config;
  document.title = `${title} · CS / Motion`;
  document.querySelector("#app").innerHTML = `
    <main class="film-page"><header class="page-header"><a class="brand" href="../index.html">${icon("M4 5h16v3H4zM4 11h16v3H4zM4 17h16v3H4z")}<span>CS <span class="brand-slash">/</span> MOTION</span></a><span class="edition">CHAPTER ${String(chapter).padStart(2, "0")} <i></i><span id="duration-edition"></span></span></header>
    <div class="title-row"><h1>${title}</h1><a class="back-link" href="./index.html">← 第 ${chapter} 章目录</a></div>
    <section class="player" aria-label="${title}动画播放器"><div class="film-viewport" id="film-viewport"><div class="film-stage" id="film-stage" role="group" aria-label="${title}教学画面"><div class="stage-light" aria-hidden="true"></div><div id="scene-host"></div><div class="film-hud" aria-hidden="true"><span><b id="hud-number">01</b><i></i><span id="hud-title"></span></span><span id="hud-view"></span></div><div class="model-note">${model}</div><div class="loading" id="loading">正在载入动画…</div></div></div>
    <div class="transport"><div class="progress-row"><output id="current-time" class="time">00.0</output><div class="seek-wrap"><div class="chapter-ticks" aria-hidden="true"></div><input id="progress" type="range" min="0" step="0.01" value="0" aria-label="动画进度"></div><span class="time total-time"><output id="total-time"></output> <span>s</span></span></div>
    <div class="button-row"><div class="playback-buttons"><button id="play" class="play-button" type="button" aria-label="播放动画">${playIcon}${pauseIcon}<span id="play-label">播放</span></button><button id="previous" class="icon-button" aria-label="上一步" title="上一步（←）">${icon("M6 5v14M18 6l-9 6 9 6")}</button><button id="next" class="icon-button" aria-label="下一步" title="播放至下一步（→）">${icon("M18 5v14M6 6l9 6-9 6")}</button><span class="control-divider"></span><button id="replay" class="text-button" title="重新播放（R）">${icon("M4 10a8 8 0 1 1 1 8M4 4v6h6")}<span>重播</span></button></div>
    <div class="playback-options"><span id="playback-status" class="playback-status"></span><button id="step-mode" class="mode-button" aria-pressed="false"><span class="switch"><i></i></span>单步讲解</button><button id="fullscreen" class="icon-button fullscreen-button" aria-label="全屏观看">${icon("M8 4H4v4m12-4h4v4M4 16v4h4m12-4v4h-4")}</button></div></div></div></section>
    <nav class="chapters" aria-label="场景导航">${scenes.map((s, i) => `<button type="button" data-chapter="${i}"><span>${String(i + 1).padStart(2, "0")}</span><div>${s.title}<small>${s.detail || ""}</small></div>${icon("m9 6 6 6-6 6")}</button>`).join("")}</nav>
    ${experimentWorkbench(config.experiment)}
    ${evidencePanel(config.provenance, !config.experiment)}
    <footer class="page-footer"><span><kbd>Space</kbd> 播放 / 暂停 <i></i><kbd>←</kbd><kbd>→</kbd> 逐步观看</span><span>${config.example ? `<a href="${config.example}">查看完整示例程序 ↗</a>` : "教学模型 · 非本机实测 · 无配音"}</span></footer><p id="accessibility-status" class="sr-only" role="status" aria-live="polite"></p></main>`;
  const $ = (selector) => document.querySelector(selector);
  $("#film-stage").dataset.source = config.provenance?.kind || "teaching-model";
  const master = gsap.timeline({ paused: true });
  // Optional same-DOM actors outlive local scene gates. Their complete lifecycle
  // still belongs to the one reversible teaching clock (no transition callbacks).
  const continuity = config.buildContinuity?.();
  const actorContext = prepareActorContinuity(config.actorContinuity);
  setActorContext(actorContext);
  let elapsed = 0;
  const chapters = scenes.map((s) => {
    const shot = s.build();
    const duration = s.duration || shot.duration || shot.timeline.duration();
    shot.timeline.timeScale(1 / stretch);
    shot.root.setAttribute("aria-label", s.title);
    master.add(shot.timeline, elapsed);
    shot.timeline.paused(false);
    const entry = {
      ...s,
      ...shot,
      start: elapsed,
      duration: duration * stretch,
    };
    elapsed += duration * stretch;
    return entry;
  });
  setActorContext(null);
  const duration = Math.round(elapsed * 1000) / 1000;
  const carriedActors = actorContext.finish(chapters, duration, stretch);
  $('#film-stage').classList.toggle('has-carried-actors', carriedActors.length > 0);
  if (continuity) {
    if (!continuity.root.matches('[data-film-continuity]') ||
        Math.abs(continuity.duration * stretch - duration) > 0.001)
      throw new Error('Invalid shared continuity layer / duration');
    continuity.timeline.timeScale(1 / stretch);
    master.add(continuity.timeline, 0);
    continuity.timeline.paused(false);
  }
  const eventTimes = config.steps
    ? config.steps.map((time) => time * stretch)
    : chapters.flatMap((s) =>
        (s.stops || [s.duration / stretch - 1]).map(
          (time) => s.start + time * stretch,
        ),
      );
  const steps = [
    ...new Set(
      [0, ...eventTimes, duration].map((n) => Math.round(n * 1000) / 1000),
    ),
  ].sort((a, b) => a - b);
  const checks = chapters.flatMap((s) =>
    (s.checks || []).map((check) => ({
      ...check,
      time: s.start + check.at * stretch,
      scene: s.root.id,
    })),
  );
  const progress = $("#progress");
  progress.max = duration;
  $("#total-time").textContent = duration.toFixed(1);
  $("#duration-edition").textContent = `${duration}-SECOND EXPLAINER`;
  $(".chapter-ticks").innerHTML = chapters
    .slice(1)
    .map((s) => `<i style="left:${(s.start / duration) * 100}%"></i>`)
    .join("");
  $(".chapters").style.gridTemplateColumns =
    `repeat(${Math.min(scenes.length, 5)},minmax(0,1fr))`;
  let playing = false,
    stepMode = false,
    stopAt = null,
    chapterIndex = -1,
    ready = false;
  let scrubbing = false,
    resumeAfterScrub = false;
  const nextStop = (time) => steps.find((t) => t > time + 0.045) ?? duration;
  const announce = (text) => {
    $("#accessibility-status").textContent = text;
  };
  function render() {
    if (!ready) return;
    let time = master.time();
    if (playing && stopAt !== null && time >= stopAt - 0.0001) {
      const target = stopAt;
      stopAt = null;
      playing = false;
      master.pause(target);
      time = target;
    }
    const index = chapters.reduce(
      (last, s, i) => (time >= s.start ? i : last),
      0,
    );
    if (index !== chapterIndex) {
      chapterIndex = index;
      chapters.forEach((s, i) => {
        s.root.hidden = i !== index;
        s.root.setAttribute("aria-hidden", String(i !== index));
      });
      document.querySelectorAll("[data-chapter]").forEach((button, i) => {
        if (i === index) button.setAttribute("aria-current", "step");
        else button.removeAttribute("aria-current");
      });
      $("#hud-number").textContent = String(index + 1).padStart(2, "0");
      $("#hud-title").textContent = chapters[index].title;
      const sceneKind =
        chapters[index].sourceKind ||
        config.provenance?.kind ||
        "teaching-model";
      $("#film-stage").dataset.sceneSource = sceneKind;
      $("#hud-view").textContent =
        `${sceneKind === "gdb-measured" ? "GDB CAPTURE" : sceneKind === "gdb-plus-model" ? "GDB + MODEL" : "MECHANISM MODEL"} / ${chapters[index].view || id.toUpperCase()}`;
      announce(chapters[index].title);
    }
    $("#current-time").textContent = time.toFixed(1).padStart(4, "0");
    progress.value = time;
    progress.style.setProperty("--progress", `${(time / duration) * 100}%`);
    progress.setAttribute(
      "aria-valuetext",
      `${time.toFixed(1)} 秒，共 ${duration} 秒；${chapters[index].title}`,
    );
    $(".player").classList.toggle("is-playing", playing);
    $("#play").setAttribute(
      "aria-label",
      playing
        ? "暂停动画"
        : time >= duration - 0.001
          ? "重新播放动画"
          : "播放动画",
    );
    $("#play-label").textContent = playing
      ? "暂停"
      : time >= duration - 0.001
        ? "重播"
        : "播放";
    $("#previous").disabled = time < 0.04;
    $("#next").disabled = time >= duration - 0.04;
    $("#step-mode").setAttribute("aria-pressed", String(stepMode));
    $("#playback-status").textContent = scrubbing
      ? "定位中"
      : playing
        ? stopAt === null
          ? "连续播放"
          : "播放至下一关键动作"
        : time >= duration - 0.001
          ? "结束 · 可回看"
          : "已暂停";
  }
  function pause() {
    playing = false;
    stopAt = null;
    master.pause();
    render();
  }
  function seek(time) {
    playing = false;
    stopAt = null;
    master.pause();
    master.time(Math.max(0, Math.min(duration, Number(time) || 0)), false);
    render();
  }
  function play() {
    if (master.time() >= duration - 0.001) master.pause(0);
    stopAt = stepMode ? nextStop(master.time()) : null;
    playing = true;
    master.play();
    render();
  }
  function next() {
    if (master.time() >= duration) return;
    stopAt = nextStop(master.time());
    playing = true;
    master.play();
    render();
  }
  function previous() {
    seek(
      [...steps].reverse().find((time) => time < master.time() - 0.045) ?? 0,
    );
  }
  function replay() {
    seek(0);
    play();
  }
  function endScrub() {
    if (!scrubbing) return;
    scrubbing = false;
    if (resumeAfterScrub && master.time() < duration) play();
    else render();
    resumeAfterScrub = false;
  }
  $("#play").onclick = () => (playing ? pause() : play());
  $("#next").onclick = next;
  $("#previous").onclick = previous;
  $("#replay").onclick = replay;
  $("#step-mode").onclick = () => {
    stepMode = !stepMode;
    if (playing) stopAt = stepMode ? nextStop(master.time()) : null;
    render();
  };
  document.querySelectorAll("[data-chapter]").forEach((button) => {
    button.onclick = () => seek(chapters[Number(button.dataset.chapter)].start);
  });
  progress.addEventListener("pointerdown", () => {
    if (!scrubbing) {
      resumeAfterScrub = playing;
      scrubbing = true;
      pause();
    }
  });
  progress.addEventListener("input", () => seek(progress.value));
  progress.addEventListener("change", endScrub);
  document.addEventListener("pointerup", endScrub);
  document.addEventListener("pointercancel", endScrub);
  progress.addEventListener("keydown", (event) => {
    let time;
    const delta = event.shiftKey ? 5 : 1;
    if (event.key === "Home") time = 0;
    else if (event.key === "End") time = duration;
    else if (["ArrowRight", "ArrowUp"].includes(event.key))
      time = master.time() + delta;
    else if (["ArrowLeft", "ArrowDown"].includes(event.key))
      time = master.time() - delta;
    else return;
    event.preventDefault();
    const resume = playing;
    seek(time);
    if (resume && master.time() < duration) play();
  });
  $("#fullscreen").onclick = async () => {
    try {
      if (document.fullscreenElement) await document.exitFullscreen();
      else await $(".player").requestFullscreen();
    } catch {
      announce("当前浏览器不支持全屏");
    }
  };
  document.addEventListener("keydown", (event) => {
    if (
      event.ctrlKey ||
      event.altKey ||
      event.metaKey ||
      event.target.closest(
        "button,input,select,summary,a,textarea,[contenteditable]",
      )
    )
      return;
    if (event.code === "Space") {
      event.preventDefault();
      playing ? pause() : play();
    } else if (event.key === "ArrowRight") {
      event.preventDefault();
      next();
    } else if (event.key === "ArrowLeft") {
      event.preventDefault();
      previous();
    } else if (event.key.toLowerCase() === "r") replay();
  });
  document.addEventListener("visibilitychange", () => {
    if (document.hidden) pause();
  });
  const resize = () => {
    $("#film-stage").style.transform =
      `scale(${$("#film-viewport").clientWidth / 1440})`;
  };
  new ResizeObserver(resize).observe($("#film-viewport"));
  resize();
  master.eventCallback("onUpdate", render);
  master.eventCallback("onComplete", () => {
    playing = false;
    stopAt = null;
    render();
    announce("动画结束，可以重播或逐步回看。");
  });
  master.time(duration, true).time(0, true);
  ready = true;
  $("#film-stage").classList.add("ready");
  render();
  const api = Object.freeze({
    id,
    duration,
    storyDuration: duration / stretch,
    timeStretch: stretch,
    dataSource: config.provenance?.kind || "teaching-model",
    continuity: continuity ? { rootId: continuity.root.id } : null,
    carriedActors,
    experimentDuration: config.experiment?.duration || 0,
    experimentCues:
      config.experiment?.cues.map(({ run, sampleIndex, time }) => ({
        run,
        sampleIndex,
        time,
      })) || [],
    steps,
    checks,
    chapters: chapters.map((s) => ({
      start: s.start,
      duration: s.duration,
      title: s.title,
      id: s.root.id,
      sourceKind: s.sourceKind || config.provenance?.kind || "teaching-model",
    })),
    play,
    pause,
    seek,
    next,
    previous,
    replay,
    getState: () => ({
      time: master.time(),
      playing,
      stepMode,
      chapter: chapterIndex,
      stopAt,
      scrubbing,
    }),
  });
  window.film = api;
  bindExperimentWorkbench(config.experiment, api);
  if (id === "stack-exhaustion") window.stackFilm = api;
  if (
    !new URLSearchParams(location.search).has("paused") &&
    !matchMedia("(prefers-reduced-motion: reduce)").matches
  )
    play();
  return api;
}
