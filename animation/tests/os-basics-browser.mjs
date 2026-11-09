import assert from "node:assert/strict";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { resolve } from "node:path";
import { createHash } from "node:crypto";
import { chromium } from "playwright";
import { createPreviewServer } from "../preview-server.mjs";
const root = resolve(import.meta.dirname, ".."),
  artifacts = resolve(root, "film-artifacts");
await mkdir(artifacts, { recursive: true });
const trace = JSON.parse(
  await readFile(resolve(root, "traces/os-basics.json"), "utf8"),
);
const films = [
  ["os-compile", "compile"],
  ["os-elf", "elf-object"],
  ["os-loading", "loading"],
  ["os-process-space", "process-space"],
  ["os-parent-child", "parent-child"],
  ["os-registers", "registers"],
];
let checks = 0;
const errors = [];
const ok = (condition, message) => {
  assert.ok(condition, message);
  checks++;
};
const eq = (actual, expected, message) => {
  assert.deepEqual(actual, expected, message);
  checks++;
};
const hex = (n) => Number(n).toString(16).toUpperCase().padStart(2, "0");
const sample = (event, run = "first") =>
  trace.runs[run].samples.find((s) => s.event === event);
const object = (s, name) => s.objects.find((o) => o.name === name);
const seek = (page, t) => page.evaluate((t) => film.seek(t), t);
const browser = await chromium.launch();
try {
  for (const published of [false, true]) {
    const server = createPreviewServer(
      published ? { directory: resolve(root, "dist"), prefix: "/csapp" } : {},
    );
    await new Promise((r) => server.listen(0, "127.0.0.1", r));
    const origin = `http://127.0.0.1:${server.address().port}`,
      base = origin + (published ? "/csapp" : "");
    try {
      const guide = await browser.newPage({
        viewport: { width: 390, height: 850 },
      });
      await guide.goto(`${base}/chapter0/guide.html`);
      eq(
        await guide.locator("article h1").count(),
        1,
        "standalone chapter has a readable HTML manuscript",
      );
      ok(
        (await guide.locator("article").textContent()).includes("可重定位"),
        "chapter manuscript is not only a topic menu",
      );
      ok(
        await guide.evaluate(
          () => document.documentElement.scrollWidth <= innerWidth,
        ),
        "reading tables/code scroll within the narrow page",
      );
      await guide.close();
      for (const artifact of trace.build.artifacts) {
        const response = await fetch(`${base}/${artifact.path}`);
        eq(response.status, 200, artifact.path + " served");
        const raw = Buffer.from(await response.arrayBuffer());
        eq(
          createHash("sha256").update(raw).digest("hex"),
          artifact.sha256,
          "published bytes retain actual file hash",
        );
      }
      for (const [id, file] of films) {
        const page = await browser.newPage({
          viewport: { width: 1440, height: 1100 },
        });
        page.on("pageerror", (e) => errors.push(e.message));
        page.on("response", (r) => {
          if (r.status() >= 400) errors.push(`${r.status()} ${r.url()}`);
        });
        page.on("request", (r) => {
          if (!r.url().startsWith(origin) && !r.url().startsWith("data:"))
            errors.push("Remote dependency " + r.url());
        });
        await page.goto(`${base}/chapter0/${file}.html?paused`);
        await page.waitForFunction(() => window.film);
        eq(
          await page.evaluate(() => film.dataSource),
          "gdb-plus-model",
          "explicit observations / model identity",
        );
        ok(
          (await page.locator(".os-lab").getAttribute("open")) !== null,
          "full program and actual debugging workbench is open",
        );
        for (const [i, source] of trace.provenance.sources.entries()) {
          await page.selectOption("#os-source", String(i));
          eq(
            await page.locator("#os-complete-source").textContent(),
            source.content,
            "complete unmodified multi-file C",
          );
        }
        for (const report of trace.reports) {
          await page.selectOption("#os-report", report.key);
          eq(
            await page.locator("#os-tool-output").textContent(),
            `$ ${report.command}\n${report.output}`,
            "actual tool output, not a paraphrase",
          );
        }
        for (const run of ["first", "second", "fork"]) {
          await page.selectOption("#os-run", run);
          const stops = trace.runs[run].samples;
          const selected =
            run === "first"
              ? stops.filter(
                  (s) =>
                    [
                      "interpreter-entry",
                      "app-entry",
                      "main-entry",
                      "layout-ready",
                      "os_add-entry",
                      "os_add-returned",
                      "compute-done",
                    ].includes(s.event) ||
                    s.previousInstruction?.asm.startsWith("cmp"),
                )
              : run === "fork"
                ? stops
                : [stops[0], stops.at(-1)];
          for (const s of selected) {
            await page.selectOption("#os-stop", String(s.index));
            eq(
              await page.locator("#os-gdb-output").textContent(),
              s.commands
                .map((c) => `(gdb) ${c.command}\n${c.output}`)
                .join("\n\n"),
              "selected actual GDB commands/output",
            );
            ok(
              (await page.locator("#os-lab-current").textContent()).includes(
                String(s.pid),
              ),
              "real selected PID",
            );
          }
        }
        for (const a of await page.locator(".os-lab-links a").all()) {
          const href = await a.getAttribute("href");
          const response = await page.request.get(
            new URL(href, page.url()).href,
          );
          eq(
            response.status(),
            200,
            "all sources/reports/ELFs/guide available",
          );
        }
        if (id === "os-compile") {
          await seek(page, 20);
          eq(
            await page
              .locator("#pp-after .code-line:last-child code")
              .textContent(),
            trace.build.preprocessed.at(-1).text,
            "macro expansion is actual -E output",
          );
          await seek(page, 47);
          eq(
            await page.locator("#as-bytes-0 .array-value").textContent(),
            trace.build.addBytes.slice(0, 4).map(hex).join(" "),
            "assembler bytes from actual .o",
          );
        } else if (id === "os-elf") {
          await seek(page, 46);
          for (let i = 0; i < 4; i++)
            eq(
              await page.locator(`#rel-bytes-${i} .array-value`).textContent(),
              hex(trace.relocation.before[i]),
              "no premature linker patch",
            );
          await seek(page, 61);
          for (let i = 0; i < 4; i++)
            eq(
              await page.locator(`#rel-bytes-${i} .array-value`).textContent(),
              hex(trace.relocation.after[i]),
              "actual four-byte PC32 patch",
            );
          await seek(page, 46);
          eq(
            await page.locator("#rel-bytes-0 .array-value").textContent(),
            hex(trace.relocation.before[0]),
            "reverse unpatches explanatory file field",
          );
        } else if (id === "os-loading") {
          await seek(page, 25);
          eq(
            await page.locator("#lb-live .reg-value").textContent(),
            sample("main-entry").registers.rip,
            "actual PIE main VA",
          );
          await seek(page, 60);
          eq(
            await page.locator("#st-interp .reg-value").textContent(),
            sample("interpreter-entry").registers.rip,
            "real dynamic interpreter PC",
          );
          eq(
            await page.locator("#st-app .reg-value").textContent(),
            sample("app-entry").registers.rip,
            "real application _start",
          );
          eq(
            await page.locator("#st-main .reg-value").textContent(),
            sample("main-entry").registers.rip,
            "real main entry",
          );
          await seek(page, 40);
          eq(
            await page.locator("#lz-data-0 .array-value").textContent(),
            hex(object(sample("layout-ready"), "data").bytes[0]),
            "real data initial byte",
          );
        } else if (id === "os-process-space") {
          await seek(page, 29);
          for (const o of sample("layout-ready").objects)
            eq(
              await page
                .locator(`#vd-array-${o.name} .array-address`)
                .textContent(),
              o.address,
              "each C object actual VA",
            );
          const names = ["data", "bss", "heap", "stack"];
          await seek(page, 45);
          for (const name of names)
            eq(
              await page
                .locator(`#ob-array-${name} .array-value`)
                .textContent(),
              String(object(sample("layout-ready"), name).value),
              "actual region value",
            );
        } else if (id === "os-parent-child") {
          const p = sample("phase-2", "fork"),
            c = sample("phase-3", "fork"),
            after = sample("phase-5", "fork"),
            changed = sample("phase-4", "fork");
          await seek(page, 25);
          eq(
            await page.locator("#ff-pr .reg-value").textContent(),
            String(c.pid),
            "parent fork returns real child PID",
          );
          eq(
            await page.locator("#ff-cr .reg-value").textContent(),
            "0",
            "child fork result",
          );
          for (const name of [
            "data",
            "bss",
            "stack",
            "heap",
            "private",
            "shared",
          ]) {
            eq(
              await page
                .locator(`#ff-parent-${name} .array-address`)
                .textContent(),
              object(p, name).address,
              "parent actual inherited VA",
            );
            eq(
              await page
                .locator(`#ff-child-${name} .array-address`)
                .textContent(),
              object(c, name).address,
              "child corresponding VA",
            );
          }
          await seek(page, 31);
          eq(
            await page.locator("#fw-child-private .array-value").textContent(),
            "40",
            "child private not prematurely changed",
          );
          await seek(page, 46);
          for (const name of [
            "data",
            "bss",
            "stack",
            "heap",
            "private",
            "shared",
          ]) {
            eq(
              await page
                .locator(`#fw-parent-${name} .array-value`)
                .textContent(),
              String(object(after, name).value),
              "parent isolated/shared result",
            );
            eq(
              await page
                .locator(`#fw-child-${name} .array-value`)
                .textContent(),
              String(object(changed, name).value),
              "child actual mutations",
            );
          }
          for (const at of [38.2, 38.5, 38.8, 39.1, 39.5, 39.8, 40, 40.2]) {
            await seek(page, at);
            const overlaps = await page.evaluate(() => {
              const p = document
                .querySelector("#fw-sync-packet")
                .getBoundingClientRect();
              const selectors =
                "#fw-code .code-line i,#fw-code .code-line code,#fw-parent .array-ident,#fw-parent .array-value,#fw-child .array-ident,#fw-child .array-value";
              return [...document.querySelectorAll(selectors)]
                .filter((el) => {
                  const range = document.createRange();
                  range.selectNodeContents(el);
                  const r = range.getBoundingClientRect();
                  return (
                    p.left < r.right &&
                    p.right > r.left &&
                    p.top < r.bottom &&
                    p.bottom > r.top
                  );
                })
                .map((el) => el.textContent);
            });
            eq(
              overlaps,
              [],
              "moving sync packet must not hide code, line numbers or memory glyphs",
            );
          }
        } else if (id === "os-registers") {
          await seek(page, 29);
          eq(
            await page.locator("#rc-stack-return .array-value").textContent(),
            sample("os_add-entry").stackTop,
            "actual saved return address",
          );
          eq(
            await page.locator("#rc-rsp .reg-value").textContent(),
            sample("os_add-entry").registers.rsp,
            "actual stack top",
          );
          await seek(page, 46);
          eq(
            await page.locator("#ra-rax .reg-value").textContent(),
            sample("os_add-returned").registers.rax,
            "real integer ABI return",
          );
          const before = trace.runs.first.samples.find((s) =>
              /\$0x5,%eax/.test(s.instruction.asm),
            ),
            after = trace.runs.first.samples.find((s) =>
              /\$0x5,%eax/.test(s.previousInstruction?.asm || ""),
            );
          await seek(page, 60);
          eq(
            await page.locator("#rw-before .reg-value").textContent(),
            before.registers.rax,
            "all-ones RAX probe",
          );
          eq(
            await page.locator("#rw-after .reg-value").textContent(),
            after.registers.rax,
            "actual zero-extended EAX write",
          );
          const flags = trace.runs.first.samples.find(
            (s) =>
              s.event.startsWith("os_equal") &&
              s.previousInstruction?.asm.startsWith("cmp"),
          );
          await seek(page, 77);
          for (const [key, bit] of [
            ["CF", 0],
            ["ZF", 6],
            ["SF", 7],
            ["OF", 11],
          ])
            eq(
              await page.locator(`#rf-${key} .reg-value`).textContent(),
              String((BigInt(flags.registers.eflags) >> BigInt(bit)) & 1n),
              "decoded real status flag",
            );
          await page.selectOption("#os-run", "first");
          await page.selectOption("#os-stop", String(after.index));
          await page.locator("#os-locate").click();
          eq(
            await page.evaluate(
              () => film.chapters[film.getState().chapter].id,
            ),
            "regs-width",
            "width probe locates related instruction explanation",
          );
        }
        const arrays = await page
          .locator(".vertical-array")
          .evaluateAll((elements) =>
            elements.map((el) => ({
              space: el.dataset.space,
              rows: [...el.querySelectorAll(".array-row")].map((row) => ({
                address: row.dataset.address,
                y: row.getBoundingClientRect().y,
              })),
              direction: el.querySelector(".array-header span").textContent,
            })),
          );
        ok(
          arrays.length > 0 || id === "os-registers",
          "new memory uses vertical arrays",
        );
        for (const a of arrays.filter((a) => a.space === "virtual")) {
          const rows = a.rows.filter((r) => /^0x/.test(r.address));
          for (let i = 1; i < rows.length; i++)
            ok(
              BigInt(rows[i - 1].address) >= BigInt(rows[i].address),
              "higher VA occupies upper row",
            );
        }
        const clipped = await page
          .locator(".vertical-array .array-address")
          .evaluateAll((elements) =>
            elements
              .filter((el) => {
                if (!el.getClientRects().length) return false;
                const r = document.createRange();
                r.selectNodeContents(el);
                return (
                  r.getBoundingClientRect().width >
                  el.getBoundingClientRect().width + 0.5
                );
              })
              .map((el) => el.textContent),
          );
        eq(clipped, [], "full long addresses, no ellipsis");
        await page.selectOption("#os-run", "first");
        const frozen = await page.evaluate(() => film.getState().time);
        await page.locator("#os-run").focus();
        await page.keyboard.press("ArrowRight");
        await page.waitForTimeout(50);
        eq(
          await page.evaluate(() => film.getState().time),
          frozen,
          "native select keys do not change film clock",
        );
        await seek(page, Math.min(9, await page.evaluate(() => film.duration)));
        await page
          .locator(".film-viewport")
          .screenshot({ path: resolve(artifacts, `${id}-program.png`) });
        await page.locator(".os-lab").scrollIntoViewIfNeeded();
        await page
          .locator(".os-lab")
          .screenshot({ path: resolve(artifacts, `${id}-lab.png`) });
        await page.setViewportSize({ width: 390, height: 850 });
        ok(
          await page.evaluate(
            () => document.documentElement.scrollWidth <= innerWidth,
          ),
          "workbench and full code do not cause page overflow",
        );
        await page.close();
      }
    } finally {
      await new Promise((r) => server.close(r));
    }
  }
  eq(errors, [], "no page errors, missing files or remote dependencies");
  await writeFile(
    resolve(artifacts, "os-basics-verification.json"),
    JSON.stringify(
      {
        checks,
        films: 6,
        runs: 3,
        samples: Object.values(trace.runs).reduce(
          (n, r) => n + r.samples.length,
          0,
        ),
        variants: ["source", "/csapp/"],
        errors,
      },
      null,
      2,
    ),
  );
  console.log(
    `PASS: ${checks} independent OS/C/ELF/GDB/memory/ABI/workbench checks; six films, source and /csapp/`,
  );
} finally {
  await browser.close();
}
