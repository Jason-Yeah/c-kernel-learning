import { escape } from "./scene-kit.js";
import { osTrace as trace } from "../models/os-basics.js";
export function attachOSLab(api, id, locations) {
  const box = document.createElement("details");
  box.className = "os-lab";
  box.open = true;
  const sources = trace.provenance.sources;
  box.innerHTML = `<summary>本章完整 C 程序、ELF 文件与实际 GDB 实验台</summary><p>三份源码组成同一个实验程序；编译／链接／readelf／objdump 与三个 GDB 运行均真实执行。网站只回放保存记录，不运行 C 或 GDB。</p><p>两个 inspect 运行彼此独立，ASLR 地址不能串用；fork 的父子属于第三次运行。CPU 时间共享、物理框、COW 内部与装载过程中的内核动作是解释模型，不是特权内核追踪。</p><div class="os-lab-links">${sources.map((s) => `<a href="../${s.path}">${s.path.split("/").pop()} ↗</a>`).join("")}<a href="../traces/os-basics.json">原始 JSON ↗</a><a href="../traces/os-basics-gdb.txt">完整 GDB 文本 ↗</a><a href="./guide.html">本章完整讲义 ↗</a></div><div class="os-lab-controls"><label>完整程序<select id="os-source">${sources.map((s, i) => `<option value="${i}">${s.path}</option>`).join("")}</select></label><label>原始工具输出<select id="os-report">${trace.reports.map((r) => `<option value="${r.key}">${r.key}</option>`).join("")}</select></label><label>GDB 运行<select id="os-run">${Object.keys(
    trace.runs,
  )
    .map((key) => `<option>${key}</option>`)
    .join(
      "",
    )}</select></label><label>实际停点<select id="os-stop"></select></label><button id="os-locate">定位对应镜头</button></div><p class="os-lab-current" id="os-lab-current"></p><div class="os-lab-columns"><div><h3>未删改完整 C／头文件</h3><pre id="os-complete-source"></pre></div><div><h3>实际执行的工具命令与原样输出</h3><pre id="os-tool-output"></pre></div><div><h3>当前停点：原样 GDB 命令与输出</h3><pre id="os-gdb-output"></pre></div><div><h3>当前停点：观测字段（地址是 VA）</h3><pre class="os-lab-observation" id="os-observation"></pre></div></div><details style="margin-top:22px"><summary>编译命令与真实文件证据（可下载，不自动执行）</summary><pre>${escape(trace.build.commands.join("\n"))}</pre><div class="os-lab-links">${trace.build.artifacts.map((a) => `<a href="../${a.path}">${a.path.split("/").pop()} (${a.size}B) ↗</a>`).join("")}${trace.reports.map((r) => `<a href="../${r.path}">${r.key} ↗</a>`).join("")}</div></details>`;
  document.querySelector(".page-footer").before(box);
  const q = (s) => box.querySelector(s),
    run = q("#os-run"),
    stop = q("#os-stop");
  let selected,
    locationKey = "source";
  const target = () =>
    locations[locationKey] ??
    (locationKey.startsWith("os_width-")
      ? locations["os_width-entry"]
      : locationKey.startsWith("os_equal-")
        ? locations["os_equal-entry"]
        : locationKey.startsWith("os_add-")
          ? locations["os_add-entry"]
          : undefined);
  const button = () => {
    q("#os-locate").textContent =
      target() === undefined
        ? "本片起点（此记录无单独对应镜头）"
        : "定位相关讲解镜头";
  };
  const source = () => {
    q("#os-complete-source").textContent =
      sources[Number(q("#os-source").value)].content;
  };
  const tool = () => {
    const r = trace.reports.find((r) => r.key === q("#os-report").value);
    q("#os-tool-output").textContent = `$ ${r.command}\n${r.output}`;
    locationKey = r.key;
    button();
  };
  const display = () => {
    selected = trace.runs[run.value].samples[Number(stop.value)];
    q("#os-gdb-output").textContent = selected.commands
      .map((c) => `(gdb) ${c.command}\n${c.output}`)
      .join("\n\n");
    const { commands, ...observed } = selected;
    q("#os-observation").textContent = JSON.stringify(observed, null, 2);
    q("#os-lab-current").textContent =
      `${run.value} · #${selected.index} ${selected.event} · PID ${selected.pid} · ${selected.source.file || "机器入口"}:${selected.source.line || "无 C 源行"}`;
    locationKey = selected.event;
    button();
  };
  const changeRun = () => {
    stop.innerHTML = trace.runs[run.value].samples
      .map(
        (s) =>
          `<option value="${s.index}">#${s.index} · ${escape(s.event)}</option>`,
      )
      .join("");
    display();
  };
  q("#os-source").onchange = () => {
    source();
    locationKey = "source";
    button();
  };
  q("#os-report").onchange = tool;
  run.onchange = changeRun;
  stop.onchange = display;
  q("#os-locate").onclick = () => {
    const at = target() ?? 0;
    api.seek(at);
    document
      .querySelector(".player")
      .scrollIntoView({ behavior: "auto", block: "start" });
  };
  source();
  tool();
  changeRun();
}
