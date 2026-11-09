import assert from "node:assert/strict";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { resolve } from "node:path";
import { chromium } from "playwright";
import { createPreviewServer } from "../preview-server.mjs";
const root = resolve(import.meta.dirname, ".."),
  artifacts = resolve(root, "film-artifacts");
await mkdir(artifacts, { recursive: true });
const topics = [
  ["buffer-copy", "stack-overflow", 3],
  ["stack-recursion", "stack-exhaustion-film", 3],
  ["cache-access", "cache", 6],
  ["process-lifecycle", "process-lifecycle", 8],
  ["fork-memory", "fork-memory-layout", 8],
  ["vm-access", "address-translation", 9],
];
const raw = Object.fromEntries(
  await Promise.all(
    topics.map(async ([id]) => [
      id,
      JSON.parse(await readFile(resolve(root, `traces/${id}.json`), "utf8")),
    ]),
  ),
);
const phase = (run, n) =>
  run.samples.find(
    (s) => s.phase === n && !["signal", "ret"].includes(s.event),
  );
const hex = (n) => "0x" + Number(n).toString(16).toUpperCase().padStart(2, "0");
let checks = 0;
const errors = [];
function eq(actual, expected, message) {
  assert.deepEqual(actual, expected, message);
  checks++;
}
function ok(condition, message) {
  assert.ok(condition, message);
  checks++;
}
function expected(id, cue, sample, trace) {
  const f = sample.fields;
  if (id === "buffer-copy") {
    const ready = phase(trace.runs.safe, 1).fields;
    if (sample.phase === 1)
      return [ready.inputAddress, ready.bufferAddress, "8 bytes"];
    if (sample.phase === 2)
      return [
        hex(f.input[f.index]),
        hex(f.memory[f.index]),
        f.index === ready.inputLength - 1 ? f.savedReturn : String(f.index),
      ];
    return [
      String(sample.pid),
      ready.savedReturn,
      String(trace.runs.safe.samples.filter((s) => s.phase === 2).length),
    ];
  }
  if (id === "stack-recursion")
    return [String(f.depth), sample.registers.rsp, `${f.blockBytes} bytes`];
  if (id === "cache-access") {
    if (sample.phase === 1)
      return [
        f.base,
        "256 bytes",
        f.offsets.map((v) => hex(v).slice(2)).join(" "),
      ];
    return [f.target, hex(f.offset), hex(f.value)];
  }
  if (id === "process-lifecycle")
    return sample.phase === 4
      ? [String(sample.pid), String(f.childPid), "waitpid 调用前"]
      : [
          String(sample.pid),
          sample.phase === 3 ? "exec 后全局重新初始化" : String(f.forkResult),
          String(f.childPid),
        ];
  if (id === "fork-memory")
    return [
      String(sample.pid),
      `${f.privateValue} @ ${f.privateAddress}`,
      `${f.sharedValue} @ ${f.sharedAddress}`,
    ];
  return [
    f.target,
    String(Number(f.page.present)),
    sample.phase === 1 ? `${f.pageSize} bytes` : hex(f.value),
  ];
}
const browser = await chromium.launch();
try {
  for (const production of [false, true]) {
    const server = createPreviewServer(
      production ? { directory: resolve(root, "dist"), prefix: "/csapp" } : {},
    );
    await new Promise((r) => server.listen(0, "127.0.0.1", r));
    const origin = `http://127.0.0.1:${server.address().port}${production ? "/csapp" : ""}`;
    try {
      for (const [id, file, chapter] of topics) {
        const trace = raw[id],
          page = await browser.newPage({
            viewport: { width: 1600, height: 1160 },
          });
        page.on("pageerror", (e) => errors.push(e.message));
        page.on("response", (r) => {
          if (r.status() >= 400) errors.push(`${r.status()} ${r.url()}`);
        });
        try {
          await page.goto(`${origin}/chapter${chapter}/${file}.html?paused`);
          await page.waitForFunction(() => window.film);
          eq(
            await page.evaluate(() => film.dataSource),
            id === "buffer-copy" ? "gdb-measured" : "gdb-plus-model",
            `${id}: observation/model boundary explicit`,
          );
          eq(
            (
              await page.locator("#experiment-source code").allTextContents()
            ).join("\n"),
            trace.provenance.source.content,
            `${id}: complete runnable C is displayed verbatim, not a different teaching fragment`,
          );
          ok(
            (await page.locator("#provenance").textContent()).includes(
              trace.provenance.source.sha256,
            ),
            `${id}: visible provenance/source hash`,
          );
          const cues = await page.evaluate(() => film.experimentCues);
          eq(
            cues.length,
            4,
            `${id}: four actual C/GDB stops, not only a download link`,
          );
          for (const [i, cue] of cues.entries()) {
            const s = trace.runs[cue.run].samples[cue.sampleIndex];
            await page.evaluate((t) => film.seek(t), cue.time - 0.02);
            eq(
              await page
                .locator(`#${id}-experiment-step-${i} .reg-value`)
                .allTextContents(),
              ["—", "—", "—"],
              `${id}: future measured values do not appear before the observation arrives`,
            );
            await page.evaluate((t) => film.seek(t), cue.time + 0.5);
            eq(
              await page
                .locator(`#${id}-experiment-step-${i} .reg-value`)
                .allTextContents(),
              expected(id, cue, s, trace),
              `${id}: film values independently match original GDB JSON`,
            );
            eq(
              await page
                .locator("#film-stage")
                .getAttribute("data-scene-source"),
              "gdb-measured",
              `${id}: experimental scene is labeled as observation`,
            );
            const shown = await page
              .locator(".experiment-step")
              .evaluateAll(
                (nodes) =>
                  nodes.filter(
                    (n) =>
                      getComputedStyle(n).visibility === "visible" &&
                      Number(getComputedStyle(n).opacity) > 0.5,
                  ).length,
              );
            eq(shown, 1, `${id}: only current experimental stop visible`);
            const sourceRows = await page
              .locator(`#${id}-experiment-step-${i} .code-line`)
              .evaluateAll((nodes) =>
                nodes.map((n) => ({
                  line: Number(n.dataset.sourceLine),
                  text: n.querySelector("code").textContent,
                })),
              );
            for (const row of sourceRows)
              eq(
                row.text,
                trace.provenance.source.content
                  .split("\n")
                  [row.line - 1].replace(/\s*\/\*.*?\*\//g, "")
                  .trimEnd(),
                `${id}: actual C source line ${row.line}`,
              );
          }
          // Workbench shows complete raw output for independent runs, including expected faults.
          for (const [key, run] of Object.entries(trace.runs)) {
            await page.selectOption("#experiment-run", key);
            const indices = [
              ...new Set([
                0,
                Math.floor(run.samples.length / 2),
                run.samples.length - 1,
              ]),
            ];
            for (const index of indices) {
              const sample = run.samples[index];
              await page.selectOption("#experiment-stop", String(index));
              eq(
                await page.locator("#experiment-command-output").textContent(),
                sample.commands
                  .map((item) => `(gdb) ${item.command}\n${item.output}`)
                  .join("\n\n"),
                `${id}/${key}/${index}: unedited actual GDB commands/output`,
              );
              ok(
                (
                  await page.locator("#experiment-current").textContent()
                ).includes(`PID ${sample.pid}`),
                `${id}: actual PID in selected record`,
              );
              if (sample.signal)
                ok(
                  (
                    await page.locator("#experiment-observation").textContent()
                  ).includes(sample.signal.address),
                  `${id}: real signal address, not a canned fault`,
                );
            }
          }
          for (const link of await page.locator(".experiment-links a").all()) {
            const response = await page.request.get(
              new URL(await link.getAttribute("href"), page.url()).href,
            );
            eq(
              response.status(),
              200,
              `${id}: C/JSON/GDB evidence resources published`,
            );
          }
          const first = cues[0];
          await page.selectOption("#experiment-run", first.run);
          await page.selectOption(
            "#experiment-stop",
            String(first.sampleIndex),
          );
          await page.locator("#experiment-locate").click();
          ok(
            Math.abs(
              (await page.evaluate(() => film.getState().time)) - first.time,
            ) < 0.001,
            `${id}: selecting a recorded stop can locate its film operation`,
          );
          if (id === "buffer-copy") {
            const record = trace.runs.safe.samples.find(
              (s) => s.phase === 2 && s.fields.index === 3,
            );
            await page.selectOption("#experiment-run", "safe");
            await page.selectOption("#experiment-stop", String(record.index));
            await page.locator("#experiment-locate").click();
            eq(
              await page.locator("#copy-safe-cell-3 b").textContent(),
              hex(record.fields.memory[3]).slice(2),
              "an individual raw GDB byte write locates its matching target cell",
            );
            ok(
              (await page.locator("#copy-safe-current").textContent()).includes(
                "input[3]",
              ),
              "sequential copy cursor still identifies the selected measured byte",
            );
            const signal = trace.runs.broken.samples.find(
              (s) => s.event === "signal",
            );
            await page.selectOption("#experiment-run", "broken");
            await page.selectOption("#experiment-stop", String(signal.index));
            await page.locator("#experiment-locate").click();
            eq(
              await page.evaluate(
                () => film.chapters[film.getState().chapter].id,
              ),
              "buffer-fetch-fault",
              "actual fault stop locates its matching fault scene",
            );
          }
          // Independent checks of the mechanism segment's directly observable values.
          const chapters = await page.evaluate(() => film.chapters);
          if (id === "buffer-copy") {
            const at =
              chapters.find((c) => c.id === "buffer-fetch-fault").start + 5.8;
            await page.evaluate((t) => film.seek(t), at);
            eq(
              await page.locator("#fault-rip .reg-value").textContent(),
              trace.runs.broken.samples.find((s) => s.event === "signal").signal
                .address,
              "actual corrupted target/fault drives final movie register",
            );
          } else if (id === "process-lifecycle") {
            const at =
              chapters.find((c) => c.id === "process-fork-returns").start + 7;
            await page.evaluate((t) => film.seek(t), at);
            eq(
              await page
                .locator("#fork-parent-return .reg-value")
                .textContent(),
              String(phase(trace.runs.early, 2).pid),
              "actual child PID drives fork animation",
            );
            await page.evaluate(
              (t) => film.seek(t),
              chapters.find((c) => c.id === "process-delayed-reap").start + 9.2,
            );
            eq(
              await page.locator("#delay-wait-value .reg-value").textContent(),
              String(phase(trace.runs.late, 6).fields.waitResult),
              "actual late-run waitpid result drives movie",
            );
          } else if (id === "cache-access") {
            const loads = trace.runs.default.samples.filter(
              (s) => s.phase === 2,
            );
            for (let i = 0; i < 8; i++) {
              const c = chapters.find((c) => c.id === `cache-access-${i}`);
              await page.evaluate(
                (t) => film.seek(t),
                c.start + c.duration - 1,
              );
              eq(
                await page.locator(`#cache-${i}-AL .reg-value`).textContent(),
                hex(loads[i].fields.value).slice(2),
                "Cache model uses observed C byte, not an invented value",
              );
            }
          } else if (id === "fork-memory") {
            await page.evaluate(
              (t) => film.seek(t),
              chapters.find((c) => c.id === "memory-code-results").start + 6.6,
            );
            const p = phase(trace.runs.default, 4).fields,
              c = phase(trace.runs.default, 3).fields;
            for (const [i, key] of [
              "globalData",
              "globalBss",
              "localValue",
              "heapValue",
              "privateValue",
              "sharedValue",
            ].entries()) {
              eq(
                await page
                  .locator(`#result-row-${i} span`)
                  .allTextContents()
                  .then((v) => v.slice(1)),
                [String(p[key]), String(c[key])],
                "six object categories come from actual GDB, not table constants",
              );
            }
          } else if (id === "stack-recursion") {
            const capacity = chapters.find((c) => c.id === "scene-capacity");
            for (const i of [0, 1, 2, 3, 1, 0, 3, 2]) {
              await page.evaluate(
                (t) => film.seek(t),
                capacity.start + ([0.55, 0.96, 1.29, 1.56][i] + 0.3) * 1.5,
              );
              const displayed = Number(
                (
                  await page
                    .locator(`#capacity-frame-${i + 4} .mini-label`)
                    .textContent()
                ).match(/\d+/)[0],
              );
              ok(
                trace.runs.exhaust.samples.some(
                  (s) =>
                    s.event !== "signal" &&
                    s.phase === 1 &&
                    s.fields.depth === displayed,
                ),
                "each compact representative corresponds to a real recorded depth",
              );
              eq(
                Number(await page.locator("#depth-count").textContent()),
                displayed,
                "discrete observed depth matches the arrived frame, including reverse seeking",
              );
            }
            await page.evaluate(
              (t) => film.seek(t),
              chapters.find((c) => c.id === "scene-capacity").start + 4.5,
            );
            eq(
              await page.locator("#depth-count").textContent(),
              String(
                trace.runs.exhaust.samples.filter(
                  (s) => s.phase === 1 && s.event !== "signal",
                ).length,
              ),
              "compressed eight visual frames do not claim maximum depth eight",
            );
          } else if (id === "vm-access") {
            await page.evaluate(
              (t) => film.seek(t),
              chapters.find((c) => c.id === "vm-write-denied").start + 8.5,
            );
            ok(
              (await page.locator("#denied-note").textContent()).includes(
                trace.runs.readonly.samples.find((s) => s.event === "signal")
                  .signal.address,
              ),
              "permission failure includes actual GDB si_addr",
            );
          }
          await page.evaluate(() =>
            film.seek(film.experimentCues[2].time + 0.5),
          );
          if (!production) {
            await page.locator(".film-viewport").screenshot({
              path: resolve(artifacts, `${file}-program-gdb.png`),
            });
            await page.locator("#experiment-workbench").screenshot({
              path: resolve(artifacts, `${file}-workbench.png`),
            });
          }
          const beforeKey = await page.evaluate(() => film.getState().time);
          await page.locator("#experiment-run").focus();
          await page.keyboard.press("ArrowRight");
          await page.waitForTimeout(60);
          eq(
            await page.evaluate(() => film.getState().time),
            beforeKey,
            "experiment selects keep native keyboard behavior without moving the film",
          );
          await page.setViewportSize({ width: 390, height: 850 });
          ok(
            await page.evaluate(
              () => document.documentElement.scrollWidth <= innerWidth,
            ),
            `${id}: full program/trace viewers do not create horizontal page overflow`,
          );
        } finally {
          await page.close();
        }
      }
    } finally {
      await new Promise((r) => server.close(r));
    }
  }
  eq(errors, [], "all experimental pages are free of script/resource errors");
  await writeFile(
    resolve(artifacts, "experiments-verification.json"),
    JSON.stringify(
      {
        checks,
        topics: topics.length,
        variants: ["source", "/csapp/"],
        errors,
      },
      null,
      2,
    ) + "\n",
  );
  console.log(
    `PASS: ${checks} independent C/GDB/animation/workbench checks for six added experiments; source and /csapp/ build`,
  );
} finally {
  await browser.close();
}
