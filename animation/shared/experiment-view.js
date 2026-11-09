import { escape } from "./scene-kit.js";

export function experimentLocation(experiment, chapters, key, sample) {
  const cue = experiment.cues.find(
    (c) => c.run === key && c.sampleIndex === sample.index,
  );
  if (cue) return { time: cue.time, exact: cue.exact !== false };
  const at = (id, offset) => {
    const chapter = chapters.find((c) => c.id === id);
    return chapter ? { time: chapter.start + offset, exact: false } : null;
  };
  const id = experiment.trace.id,
    phase = sample.phase;
  if (id === "buffer-copy") {
    if (sample.event === "signal") return at("buffer-fetch-fault", 5.8);
    if (sample.event === "ret")
      return key === "broken"
        ? at("buffer-ret-use", 3)
        : at(`buffer-${key}`, 7.5);
    const scene = key === "broken" ? "buffer-corruption" : `buffer-${key}`;
    if (phase === 1) return at(scene, 0.56);
    if (phase === 2)
      return at(
        scene,
        key === "broken"
          ? 1.18 + sample.fields.index * 0.4 + 0.01
          : 1.31 + sample.fields.index * 0.48 + 0.01,
      );
    if (phase === 3) return at(scene, key === "broken" ? 12.1 : 5.2);
    if (phase === 4) return at(scene, key === "safe" ? 8.3 : 6.8);
  }
  if (id === "cache-access" && phase === 2) {
    const chapter = chapters.find(
      (c) => c.id === `cache-access-${sample.fields.step}`,
    );
    return chapter
      ? { time: chapter.start + chapter.duration - 1, exact: false }
      : null;
  }
  if (id === "stack-recursion") {
    if (sample.event === "signal") return at("scene-overflow", 4);
    if (
      key === "exhaust" &&
      sample.fields.depth ===
        experiment.trace.runs.exhaust.samples.filter(
          (s) => s.phase === 1 && s.event !== "signal",
        ).length
    )
      return at("scene-capacity", 4.5);
    if (key === "bounded" && phase === 3) return at("scene-return", 6.5);
  }
  if (id === "process-lifecycle") {
    if (key === "late") {
      if (phase === 1 || phase === 2) return at("process-child-first", 2);
      if (sample.event === "exec" || phase === 3)
        return at("process-child-first", 4.5);
      if (phase === 4 || phase === 5) return at("process-delayed-reap", 6.5);
      if (phase === 6) return at("process-delayed-reap", 9.2);
    } else {
      if (sample.event === "exec") return at("process-early-wait", 7.2);
      if (phase === 6) return at("process-exit-reap", 6.9);
    }
  }
  if (id === "vm-access") {
    if (key === "readonly")
      return at("vm-write-denied", sample.event === "signal" ? 8.5 : 2.5);
    if (key === "unmapped")
      return at("vm-unmapped", sample.event === "signal" ? 8.7 : 3.6);
    if (key === "resident-b")
      return at("vm-resident-miss", phase === 2 ? 8.5 : 0.5);
  }
  return null;
}
export function experimentWorkbench(experiment) {
  if (!experiment) return "";
  const { trace } = experiment;
  const p = trace.provenance;
  return `<details class="experiment-workbench" id="experiment-workbench" open><summary>完整 C 程序与实际 GDB 调试记录</summary><p>这是本机已采集的一次离线回放。选择运行与停点，可核对实际源码、命令和结果；网页不执行 C 或控制 GDB。</p><p class="experiment-scope">${escape(experiment.scope)}</p><div class="experiment-links"><a href="../${p.source.path}">完整 C 源码 ↗</a><a href="../traces/${trace.id}.json">原始 JSON ↗</a><a href="../traces/${trace.id}-gdb.txt">完整 GDB 文本 ↗</a>${experiment.legacyExample ? `<a href="${escape(experiment.legacyExample)}">原有学习示例（不是本次采集程序）↗</a>` : ""}</div><div class="experiment-toolbar"><label>独立运行 <select id="experiment-run" aria-label="选择实验运行">${Object.keys(
    trace.runs,
  )
    .map((key) => `<option value="${escape(key)}">${escape(key)}</option>`)
    .join(
      "",
    )}</select></label><label>实际停点 <select id="experiment-stop" aria-label="选择 GDB 实测停点"></select></label><button type="button" id="experiment-locate">定位实验镜头</button></div><p id="experiment-current" class="experiment-current"></p><div class="experiment-columns"><div><h3>${escape(p.source.path)} · 完整程序</h3><div class="experiment-source-scroll" id="experiment-source-scroll"><pre id="experiment-source">${p.source.content
    .split("\n")
    .map(
      (text, i) =>
        `<span data-source-line="${i + 1}"><i>${i + 1}</i><code>${escape(text)}</code></span>`,
    )
    .join(
      "",
    )}</pre></div></div><div><h3>原样执行的 GDB 命令与输出</h3><pre id="experiment-command-output" class="experiment-command-output"></pre><p id="experiment-observation"></p></div></div></details>`;
}

export function bindExperimentWorkbench(experiment, api) {
  if (!experiment) return;
  const { trace } = experiment;
  const runSelect = document.querySelector("#experiment-run");
  const stopSelect = document.querySelector("#experiment-stop");
  let current;
  function display() {
    const key = runSelect.value,
      sample = trace.runs[key].samples[Number(stopSelect.value)];
    current = { key, sample };
    document.querySelector("#experiment-command-output").textContent =
      sample.commands
        .map((item) => `(gdb) ${item.command}\n${item.output}`)
        .join("\n\n");
    const line = sample.callsite.line || sample.source.line;
    document.querySelector("#experiment-current").textContent =
      `${key} · 停点 #${sample.index} · PID ${sample.pid} · ${sample.event} · GDB 源行 ${sample.source.line || "无源码（机器指令／信号停点）"}${line && line !== sample.source.line ? `，调用位置 ${line}` : ""}`;
    const signal = sample.signal
      ? `${sample.signal.name} / si_code=${sample.signal.code} / si_addr=${sample.signal.address}`
      : "";
    document.querySelector("#experiment-observation").textContent =
      signal ||
      `真实寄存器：rsp=${sample.registers.rsp}，rbp=${sample.registers.rbp}，rip=${sample.registers.rip}。`;
    for (const row of document.querySelectorAll("#experiment-source > span"))
      row.classList.toggle(
        "current-source-line",
        Number(row.dataset.sourceLine) === line,
      );
    const row = document.querySelector(
      `#experiment-source > span[data-source-line="${line}"]`,
    );
    if (row)
      document.querySelector("#experiment-source-scroll").scrollTop = Math.max(
        0,
        row.offsetTop - 80,
      );
    const location = experimentLocation(experiment, api.chapters, key, sample);
    document.querySelector("#experiment-locate").textContent = location?.exact
      ? "定位这个实验停点"
      : location
        ? "定位对应机制段（解释性演示）"
        : "查看实验概览（本停点保留完整原始记录）";
  }
  function chooseRun() {
    stopSelect.innerHTML = trace.runs[runSelect.value].samples
      .map(
        (sample) =>
          `<option value="${sample.index}">#${sample.index} · ${escape(sample.event)} · PID ${sample.pid}</option>`,
      )
      .join("");
    display();
  }
  runSelect.addEventListener("change", chooseRun);
  stopSelect.addEventListener("change", display);
  document.querySelector("#experiment-locate").addEventListener("click", () => {
    const location = experimentLocation(
      experiment,
      api.chapters,
      current.key,
      current.sample,
    );
    api.seek(location?.time || 0);
    document
      .querySelector(".player")
      .scrollIntoView({ block: "start", behavior: "instant" });
  });
  chooseRun();
}
