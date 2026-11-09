import assert from "node:assert/strict";
import { readFile, mkdir } from "node:fs/promises";
import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { chromium } from "playwright";
import { createPreviewServer } from "../preview-server.mjs";
const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
// Deliberately read raw capture independently, not the renderer's model/expected-value helpers.
const raw = JSON.parse(
  await readFile(resolve(root, "traces/stack-call.json"), "utf8"),
);
const event = (name) => raw.samples[raw.events[name]];
const bindings = [
  [
    5.5,
    "#call-source .code-line:last-child i",
    String(event("main_before_call").source.line),
  ],
  [
    22,
    "#nested-code .code-line i",
    String(event("caller_before_add").source.line),
  ],
  [
    5.5,
    "#parameter-rdi .reg-value",
    String(Number(BigInt(event("main_before_call").registers.rdi))),
  ],
  [10, "#caller-rsp .reg-value", event("caller_entry").registers.rsp],
  [13.5, "#caller-rsp .reg-value", event("caller_frame_ready").registers.rsp],
  [13.5, "#caller-rbp .reg-value", event("caller_frame_ready").registers.rbp],
  [13.5, "#caller-slot-0 code", event("caller_entry").stack[0].value],
  [19.2, "#nested-rsp .reg-value", event("add_entry").registers.rsp],
  [22, "#nested-rsp .reg-value", event("add_frame_ready").registers.rsp],
  [22, "#nested-rbp .reg-value", event("add_frame_ready").registers.rbp],
  [22, "#nested-add .frame-content code", event("add_entry").stack[0].value],
  [
    28.8,
    "#sum-rax .reg-value",
    String(Number(BigInt(event("add_result_ready").registers.rax))),
  ],
  [33.8, "#return-add-rsp .reg-value", event("add_ret").registers.rsp],
  [33.8, "#return-add-rip .reg-value", event("add_ret").registers.rip],
  [37.2, "#return-add-rsp .reg-value", event("caller_after_add").registers.rsp],
  [37.2, "#return-add-rip .reg-value", event("caller_after_add").registers.rip],
  [42.1, "#return-main-rsp .reg-value", event("caller_ret").registers.rsp],
  [46.5, "#return-main-rsp .reg-value", event("main_after_call").registers.rsp],
  [46.5, "#return-main-rbp .reg-value", event("main_after_call").registers.rbp],
];
const browser = await chromium.launch();
let assertions = 0;
try {
  for (const [directory, prefix] of [
    [root, ""],
    [resolve(root, "dist"), "/csapp"],
  ]) {
    const server = createPreviewServer({ directory, prefix });
    await new Promise((r) => server.listen(0, "127.0.0.1", r));
    const origin = `http://127.0.0.1:${server.address().port}`;
    const context = await browser.newContext({
      viewport: { width: 1600, height: 1160 },
    });
    try {
      const page = await context.newPage();
      const errors = [];
      page.on("pageerror", (e) => errors.push(e.message));
      await page.goto(`${origin}${prefix}/chapter3/stack-frame.html?paused`);
      await page.waitForFunction(() => window.film);
      assert.equal(await page.evaluate(() => film.dataSource), "gdb-measured");
      assertions++;
      for (const [time, selector, value] of [
        ...bindings,
        ...[...bindings].reverse(),
      ]) {
        await page.evaluate((t) => film.seek(t), time);
        assert.equal(
          (await page.locator(selector).textContent()).trim(),
          value,
          `${prefix || "source"} at ${time}: ${selector} must match raw GDB`,
        );
        assertions++;
      }
      await page.locator("#provenance > summary").click();
      assert.ok(
        (await page.locator("#provenance").textContent()).includes(
          raw.provenance.source.sha256,
        ),
      );
      assertions++;
      assert.ok(
        (await page.locator("#provenance").textContent()).includes(
          raw.provenance.binary.sha256,
        ),
      );
      assertions++;
      for (const [href, expected] of [
        ["../experiments/stack-call.c", raw.provenance.source.content],
        ["../traces/stack-call-gdb.txt", null],
        ["../traces/stack-call.json", null],
      ]) {
        const link = page.locator(`#provenance a[href="${href}"]`);
        assert.equal(await link.count(), 1);
        const response = await page.request.get(new URL(href, page.url()).href);
        assert.equal(response.status(), 200);
        if (expected !== null) assert.equal(await response.text(), expected);
        if (href.endsWith(".json"))
          assert.equal(
            (await response.json()).provenance.source.sha256,
            raw.provenance.source.sha256,
          );
        if (href.endsWith(".txt"))
          assert.ok(
            (await response.text())
              .toLowerCase()
              .includes(event("main_before_call").registers.rsp.toLowerCase()),
          );
        assertions += 3;
      }
      await page.setViewportSize({ width: 390, height: 844 });
      assert.ok(
        await page.evaluate(
          () => document.documentElement.scrollWidth <= innerWidth,
        ),
        "expanded provenance panel must wrap on narrow screens",
      );
      assertions++;
      await page.setViewportSize({ width: 1600, height: 1160 });
      await page.evaluate(() => film.seek(13.5));
      await mkdir(resolve(root, "film-artifacts"), { recursive: true });
      await page.screenshot({
        path: resolve(
          root,
          "film-artifacts",
          prefix ? "measured-published.png" : "measured-source.png",
        ),
        fullPage: true,
      });
      assert.deepEqual(errors, []);
      assertions++;
    } finally {
      await context.close();
      await new Promise((r) => server.close(r));
    }
  }
  console.log(
    `PASS: ${assertions} independent raw-GDB/render/provenance checks; source and /csapp/ build`,
  );
} finally {
  await browser.close();
}
