import assert from "node:assert/strict";
import { mkdir, writeFile } from "node:fs/promises";
import { resolve } from "node:path";
import { chromium } from "playwright";
import { createPreviewServer } from "../preview-server.mjs";
import raw from "../traces/buffer-copy.json" with { type: "json" };

const root = resolve(import.meta.dirname, "..");
const artifacts = resolve(root, "film-artifacts");
await mkdir(artifacts, { recursive: true });
const browser = await chromium.launch();
let checks = 0;
const errors = [];
const equal = (actual, expected, message) => {
  assert.deepEqual(actual, expected, message);
  checks++;
};
const ok = (value, message) => {
  assert.ok(value, message);
  checks++;
};
const hex = (byte) =>
  byte === null ? "?" : byte.toString(16).toUpperCase().padStart(2, "0");
const cases = [
  {
    key: "safe",
    prefix: "copy-safe",
    start: 8,
    first: 0.65,
    gap: 0.48,
    duration: 0.58,
    total: 8,
  },
  {
    key: "edge",
    prefix: "copy-edge",
    start: 18,
    first: 0.65,
    gap: 0.48,
    duration: 0.58,
    total: 9,
  },
  {
    key: "broken",
    prefix: "broken",
    start: 26,
    first: 0.6,
    gap: 0.4,
    duration: 0.5,
    total: 32,
  },
];

async function sample(page, time) {
  await page.evaluate((t) => film.seek(t), time);
  return page.evaluate(() => {
    const scene = document.querySelector(".scene:not([hidden])");
    const source = [...scene.querySelectorAll(".buffer-source-hex")].map(
      (el) => el.textContent,
    );
    const target = [...scene.querySelectorAll(".buffer-target-cell b")].map(
      (el) => el.textContent,
    );
    return { source, target };
  });
}

try {
  for (const production of [false, true]) {
    const server = createPreviewServer(
      production ? { directory: resolve(root, "dist"), prefix: "/csapp" } : {},
    );
    await new Promise((r) => server.listen(0, "127.0.0.1", r));
    const origin = `http://127.0.0.1:${server.address().port}${production ? "/csapp" : ""}`;
    const page = await browser.newPage({
      viewport: { width: 1440, height: 1050 },
    });
    page.on("pageerror", (e) => errors.push(e.message));
    page.on("response", (r) => {
      if (r.status() >= 400) errors.push(`${r.status()} ${r.url()}`);
    });
    try {
      await page.goto(`${origin}/chapter3/stack-overflow.html?paused`);
      await page.waitForFunction(() => window.film);
      equal(
        await page.evaluate(() => film.dataSource),
        "gdb-measured",
        "buffer animation now uses the actually captured C/GDB evidence",
      );
      const offset = await page.evaluate(() => film.experimentDuration);
      for (const config of cases) {
        const c = { ...config, start: config.start + offset };
        const run = raw.runs[c.key];
        const ready = run.samples.find((s) => s.phase === 1).fields;
        const trace = run.samples
          .filter((s) => s.phase === 2 && s.event !== "signal")
          .map((s) => s.fields);
        const input = ready.input.map(hex),
          initial = ready.memory.slice(0, c.total).map(hex);
        const first = await sample(page, c.start + 0.56);
        equal(
          first,
          { source: input, target: initial },
          `${c.key}: original input and actually initialized buf are separate before copying`,
        );
        const geometry = await page.evaluate((prefix) => {
          const scene = document.querySelector(".scene:not([hidden])");
          const boundary = scene
            .querySelector(`#${prefix}-boundary`)
            .getBoundingClientRect();
          const valid = [
            ...scene.querySelectorAll(".buffer-target-cell.in-buf"),
          ];
          const outside = [
            ...scene.querySelectorAll(".buffer-target-cell.outside-buf"),
          ];
          const stage = document
              .querySelector("#film-stage")
              .getBoundingClientRect(),
            scale = stage.width / 1440;
          const paths = [...scene.querySelectorAll(".flow-path")].map(
            (path, i) => {
              const a = path.getPointAtLength(0),
                b = path.getPointAtLength(path.getTotalLength());
              const source = scene
                .querySelector(`#${prefix}-source-${i}`)
                .getBoundingClientRect();
              const target = scene
                .querySelectorAll(".buffer-target-cell")
                [i].getBoundingClientRect();
              return {
                originError: Math.abs(
                  a.x - (source.left + source.width / 2 - stage.left) / scale,
                ),
                targetError: Math.abs(
                  b.x - (target.left + target.width / 2 - stage.left) / scale,
                ),
                sourceGap: a.y - (source.bottom - stage.top) / scale,
                targetGap: (target.top - stage.top) / scale - b.y,
              };
            },
          );
          const labels = [
            ...scene.querySelectorAll(
              ".buffer-source-label,.buffer-target-label",
            ),
          ];
          return {
            count: valid.length,
            validEnclosed: valid.every((el) => {
              const r = el.getBoundingClientRect();
              return r.left > boundary.left && r.right < boundary.right;
            }),
            outsideExcluded: outside.every(
              (el) => el.getBoundingClientRect().left > boundary.right,
            ),
            labels: labels.map((el) => ({
              text: el.textContent,
              shown:
                getComputedStyle(el).visibility === "visible" &&
                Number(getComputedStyle(el).opacity) > 0.9,
            })),
            paths,
          };
        }, c.prefix);
        equal(
          geometry.count,
          8,
          `${c.key}: input length never changes buf capacity`,
        );
        ok(
          geometry.validEnclosed && geometry.outsideExcluded,
          `${c.key}: boundary contains only the eight original buf slots`,
        );
        ok(
          geometry.labels.length === 2 && geometry.labels.every((l) => l.shown),
          `${c.key}: source and destination labels persist`,
        );
        ok(
          geometry.labels[0].text.includes("input") &&
            geometry.labels[1].text.includes("buf[8]"),
          `${c.key}: roles are explicit, not color-only`,
        );
        for (const path of geometry.paths) {
          ok(
            path.originError < 0.1 &&
              path.targetError < 0.1 &&
              path.sourceGap >= 0 &&
              path.sourceGap <= 15 &&
              path.targetGap >= 7 &&
              path.targetGap <= 10,
            `${c.key}: each copy originates from its own input cell and lands at its matching destination`,
          );
        }
        const frames = [];
        trace.forEach((r, i) => {
          const hit = c.start + c.first + i * c.gap + c.duration + 0.08;
          const before =
            i === 0 ? initial : trace[i - 1].memory.slice(0, c.total).map(hex);
          frames.push({ time: hit - 0.01, target: before });
          frames.push({
            time: hit + 0.01,
            target: r.memory.slice(0, c.total).map(hex),
          });
        });
        for (const direction of [frames, [...frames].reverse()]) {
          for (const frame of direction) {
            const actual = await sample(page, frame.time);
            equal(
              actual.source,
              input,
              `${c.key}: copying never removes or mutates original input`,
            );
            equal(
              actual.target,
              frame.target,
              `${c.key}: only destination commits on byte arrival, including reverse seek`,
            );
          }
        }
      }
      await page.evaluate(() => film.seek(film.experimentDuration + 23.6));
      await page.locator(".film-viewport").screenshot({
        path: resolve(
          artifacts,
          production
            ? "buffer-source-target-published.png"
            : "buffer-source-target.png",
        ),
      });
      await page.setViewportSize({ width: 390, height: 850 });
      ok(
        await page.evaluate(
          () => document.documentElement.scrollWidth <= innerWidth,
        ),
        "narrow-screen controls and stage do not create page overflow",
      );
    } finally {
      await page.close();
      await new Promise((r) => server.close(r));
    }
  }
  equal(
    errors,
    [],
    "source and published copies have no script/resource errors",
  );
  await writeFile(
    resolve(artifacts, "buffer-copy-verification.json"),
    JSON.stringify(
      {
        checks,
        copies: 44,
        variants: ["source", "/csapp/"],
        forwardAndReverse: true,
        errors,
      },
      null,
      2,
    ) + "\n",
  );
  console.log(
    `PASS: ${checks} buffer identity/capacity/copy/seek checks; all 44 writes, source and /csapp/ build`,
  );
} finally {
  await browser.close();
}
