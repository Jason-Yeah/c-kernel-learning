import assert from "node:assert/strict";
import { readFile, writeFile, mkdir } from "node:fs/promises";
import { createHash } from "node:crypto";
import { resolve } from "node:path";
import { chromium } from "playwright";
import { createPreviewServer } from "../preview-server.mjs";
const root = resolve(import.meta.dirname, ".."),
  raw = JSON.parse(
    await readFile(resolve(root, "traces/instruction-cycle.json"), "utf8"),
  );
const event = (n) => raw.samples[raw.events[n]],
  hex = (b) => b.toString(16).toUpperCase().padStart(2, "0");
let checks = 0;
const errors = [];
const eq = (a, b, text) => {
    assert.deepEqual(a, b, text);
    checks++;
  },
  ok = (v, text) => {
    assert.ok(v, text);
    checks++;
  };
const seek = (page, t) => page.evaluate((t) => film.seek(t), t);
const bindings = [
  [10, "#ic-data-out .array-value", "0"],
  [24, "#if-rip .reg-value", event("load").registers.rip],
  [
    24,
    "#if-bytes .reg-value",
    event("load").instruction.bytes.map(hex).join(" "),
  ],
  [37, "#id-ea .reg-value", event("load").inputs.address],
  [37, "#id-eax .reg-value", event("loaded").registers.eax],
  [42, "#ie-eax .reg-value", event("add").registers.eax],
  [42, "#ie-edx .reg-value", event("add").registers.edx],
  [52, "#ie-result .reg-value", event("added").registers.eax],
  [52, "#ie-rip .reg-value", event("added").registers.rip],
  [58, "#iw-output-0 .array-value", hex(event("store").output.bytes[0])],
  [63, "#iw-output-0 .array-value", hex(event("store").output.bytes[0])],
  ...event("stored").output.bytes.map((v, i) => [
    68,
    `#iw-output-${i} .array-value`,
    hex(v),
  ]),
  [82, "#in-ret .reg-value", event("returned").registers.rip],
  [82, "#in-rsp .reg-value", event("returned").registers.rsp],
  [82, "#in-value .reg-value", event("returned").registers.rax],
  [94, "#is-observed .reg-value", String(event("stored").output.value)],
];
const browser = await chromium.launch();
await mkdir(resolve(root, "film-artifacts"), { recursive: true });
try {
  for (const published of [false, true]) {
    const server = createPreviewServer(
      published ? { directory: resolve(root, "dist"), prefix: "/csapp" } : {},
    );
    await new Promise((r) => server.listen(0, "127.0.0.1", r));
    const origin = `http://127.0.0.1:${server.address().port}`,
      base = origin + (published ? "/csapp" : "");
    try {
      const page = await browser.newPage({
        viewport: { width: 1440, height: 1100 },
      });
      page.on("pageerror", (e) => errors.push(e.message));
      page.on("response", (r) => {
        if (r.status() >= 400) errors.push(r.status() + " " + r.url());
      });
      page.on("request", (r) => {
        if (!r.url().startsWith(origin) && !r.url().startsWith("data:"))
          errors.push("Remote " + r.url());
      });
      await page.goto(base + "/chapter1/instruction-cycle.html?paused");
      await page.waitForFunction(() => window.film);
      eq(
        await page.evaluate(() => film.duration),
        96,
        "actual authoring duration",
      );
      eq(
        await page.evaluate(() => film.chapters.length),
        7,
        "seven independent compositions",
      );
      eq(
        await page.evaluate(() => film.dataSource),
        "gdb-plus-model",
        "architectural facts / internal model boundary",
      );
      const displayedSource = (
        await page.locator("#experiment-source > span code").allTextContents()
      ).join("\n");
      eq(
        displayedSource,
        raw.provenance.source.content,
        "complete C program, not a replacement snippet",
      );
      for (const [time, selector, expected] of [
        ...bindings,
        ...bindings.toReversed(),
      ]) {
        await seek(page, time);
        eq(
          (await page.locator(selector).textContent()).trim(),
          expected,
          "direct raw-GDB match " + selector + " at " + time,
        );
      }
      for (const time of [13, 18, 24, 27, 35, 39]) {
        await seek(page, time);
        for (const [i, v] of event("load").instruction.bytes.entries())
          eq(
            await page.locator(`#if-code-${i} .array-value`).textContent(),
            hex(v),
            "fetch keeps its code source intact",
          );
      }
      await seek(page, 57);
      eq(
        await page.locator("#iw-output .array-row").count(),
        4,
        "fixed output capacity",
      );
      for (const time of [58, 60, 63, 68, 71, 58]) {
        await seek(page, time);
        for (const i of [0, 1])
          eq(
            await page.locator(`#iw-input-${i} .array-value`).textContent(),
            String(event("store").inputs.values[i]),
            "input bytes never consumed or overwritten",
          );
      }
      // The generic workbench must expose every actual instruction, including omitted prologue/spills.
      for (const s of raw.samples) {
        await page.selectOption("#experiment-stop", String(s.index));
        eq(
          await page.locator("#experiment-command-output").textContent(),
          s.commands.map((c) => `(gdb) ${c.command}\n${c.output}`).join("\n\n"),
          "every selected original command/output",
        );
        ok(
          (await page.locator("#experiment-current").textContent()).includes(
            String(s.pid),
          ),
          "real PID",
        );
        const after = raw.samples[s.index + 1];
        if (after)
          ok(
            (await page.locator("#cycle-step-effects").textContent()).includes(
              `${s.registers.rip} → ${after.registers.rip}`,
            ),
            "explicit real before/after transition",
          );
      }
      await page.selectOption("#experiment-stop", String(raw.events.stored));
      await page.locator("#experiment-locate").click();
      eq(
        await page.evaluate(() => film.chapters[film.getState().chapter].id),
        "cycle-writeback",
        "store stop locates related writeback explanation",
      );
      ok(
        (await page.locator("#experiment-locate").textContent()).includes(
          "解释性",
        ),
        "not advertised as hardware CPU recording",
      );
      const p = raw.provenance;
      for (const [href, sha] of [
        [`../${p.source.path}`, p.source.sha256],
        [`../${p.binary.path}`, p.binary.sha256],
      ]) {
        const response = await page.request.get(new URL(href, page.url()).href);
        eq(response.status(), 200, "evidence available");
        eq(
          createHash("sha256")
            .update(await response.body())
            .digest("hex"),
          sha,
          "actual evidence hash",
        );
      }
      for (const link of await page.locator(".experiment-links a").all()) {
        eq(
          (
            await page.request.get(
              new URL(await link.getAttribute("href"), page.url()).href,
            )
          ).status(),
          200,
          "all local documents and evidence links",
        );
      }
      for (const time of [
        14.5, 15, 16, 17.5, 18.8, 31, 33.5, 34, 47, 49, 59.5, 61, 62.5, 75, 79,
        80,
      ]) {
        await seek(page, time);
        const collisions = await page.evaluate(() => {
          const scene = document.querySelector(".scene:not([hidden])"),
            packets = [...scene.querySelectorAll(".moving-packet")].filter(
              (el) => {
                const s = getComputedStyle(el);
                return s.visibility !== "hidden" && Number(s.opacity) > 0.5;
              },
            );
          const walker = document.createTreeWalker(scene, NodeFilter.SHOW_TEXT),
            text = [];
          while (walker.nextNode()) {
            const node = walker.currentNode;
            if (!node.textContent.trim() || node.parentElement.closest("svg"))
              continue;
            let visible = true;
            for (
              let el = node.parentElement;
              el && el !== scene;
              el = el.parentElement
            ) {
              const s = getComputedStyle(el);
              if (
                s.visibility === "hidden" ||
                s.display === "none" ||
                Number(s.opacity) < 0.5
              ) {
                visible = false;
                break;
              }
            }
            if (visible) {
              const range = document.createRange();
              range.selectNodeContents(node);
              for (const rect of range.getClientRects())
                text.push({ text: node.textContent, rect });
            }
          }
          return packets.flatMap((p) => {
            const a = p.getBoundingClientRect();
            return text
              .filter(
                ({ rect: b }) =>
                  a.left < b.right - 1 &&
                  a.right > b.left + 1 &&
                  a.top < b.bottom - 1 &&
                  a.bottom > b.top + 1,
              )
              .map((t) => [p.id, t.text]);
          });
        });
        eq(
          collisions,
          [],
          "moving payload does not hide visible glyphs at " + time,
        );
      }
      const frozen = await page.evaluate(() => film.getState().time);
      await page.locator("#experiment-stop").focus();
      await page.keyboard.press("ArrowRight");
      await page.waitForTimeout(50);
      eq(
        await page.evaluate(() => film.getState().time),
        frozen,
        "native stop select keyboard does not move film",
      );
      await seek(page, 34);
      await page
        .locator(".film-viewport")
        .screenshot({
          path: resolve(
            root,
            "film-artifacts",
            published
              ? "instruction-cycle-action-published.png"
              : "instruction-cycle-action.png",
          ),
        });
      await page.locator("#experiment-workbench").scrollIntoViewIfNeeded();
      await page
        .locator("#experiment-workbench")
        .screenshot({
          path: resolve(
            root,
            "film-artifacts",
            published
              ? "instruction-cycle-lab-published.png"
              : "instruction-cycle-lab.png",
          ),
        });
      await page.setViewportSize({ width: 390, height: 850 });
      ok(
        await page.evaluate(
          () => document.documentElement.scrollWidth <= innerWidth,
        ),
        "full program/effects panel wraps inside narrow page",
      );
      await page.close();
    } finally {
      await new Promise((r) => server.close(r));
    }
  }
  eq(errors, [], "no errors, missing files or CDN");
  await writeFile(
    resolve(root, "film-artifacts/instruction-cycle-verification.json"),
    JSON.stringify(
      {
        checks,
        stops: raw.samples.length,
        variants: ["source", "/csapp/"],
        errors,
      },
      null,
      2,
    ),
  );
  console.log(
    `PASS: ${checks} independent instruction-byte/state/writeback/workbench/visibility checks; source and /csapp/`,
  );
} finally {
  await browser.close();
}
