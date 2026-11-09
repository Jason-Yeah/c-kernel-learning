import assert from "node:assert/strict";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { chromium } from "playwright";
import { createPreviewServer } from "../preview-server.mjs";
import { films, chapters as catalogChapters } from "../shared/catalog.js";
const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const artifacts = resolve(root, "film-artifacts");
await mkdir(artifacts, { recursive: true });
const server = createPreviewServer();
await new Promise((r) => server.listen(0, "127.0.0.1", r));
const origin = `http://127.0.0.1:${server.address().port}`;
const browser = await chromium.launch();
let assertions = 0;
const errors = [],
  badResources = [],
  bounds = [];
const check = (condition, message) => {
  assert.ok(condition, message);
  assertions++;
};
function monitor(page, allowed = origin) {
  page.on("pageerror", (e) => errors.push(e.message));
  page.on("response", (r) => {
    if (r.status() >= 400) badResources.push(`${r.status()} ${r.url()}`);
  });
  page.on("requestfailed", (r) => badResources.push(r.url()));
  page.on("request", (r) => {
    if (!r.url().startsWith(allowed) && !r.url().startsWith("data:"))
      errors.push(`External resource: ${r.url()}`);
  });
}
const seek = (page, time) => page.evaluate((t) => film.seek(t), time);
const clock = (page) => page.evaluate(() => film.getState());
async function shown(page, selector) {
  return page.locator(selector).evaluate((el) => {
    if (!el.getClientRects().length) return false;
    for (let n = el; n instanceof Element; n = n.parentElement) {
      const s = getComputedStyle(n);
      if (
        s.display === "none" ||
        s.visibility === "hidden" ||
        Number(s.opacity) < 0.01
      )
        return false;
    }
    return true;
  });
}
async function presentation(page, label) {
  const style = await page.evaluate(() => {
    const css = (el) => getComputedStyle(el);
    const backgrounds = [
      "body",
      ".film-viewport",
      "#film-stage",
      ".transport",
    ].map((selector) => {
      const s = css(document.querySelector(selector));
      return { selector, color: s.backgroundColor, image: s.backgroundImage };
    });
    const ordinary = [
      ...document.querySelectorAll(
        ".reg-label,.reg-name,.reg-detail,.anchored-note > span,.anchored-note:not(.red-note) > strong,.code-heading,.eyebrow:not([style]),.buffer-lane-label > span,.buffer-unknown,.buffer-encoding",
      ),
    ];
    const wrongText = ordinary
      .filter((el) => css(el).color !== "rgb(255, 255, 255)")
      .map((el) => el.id || el.className);
    const names = [...document.querySelectorAll(".reg-label")].filter((el) =>
      el.textContent.trim().startsWith("%"),
    );
    const namedRegisters = names.every((el) => {
      const name = el.querySelector(".reg-name");
      return (
        name &&
        parseFloat(css(name).fontSize) >= 26 &&
        Number(css(name).fontWeight) >= 600
      );
    });
    const terms = [...document.querySelectorAll(".reg-label")].every(
      (el) => parseFloat(css(el).fontSize) >= 20,
    );
    const clippedAddresses = [
      ...document.querySelectorAll(".measured-address .reg-value"),
    ]
      .filter((el) => {
        if (!el.getClientRects().length || css(el).visibility === "hidden")
          return false;
        const range = document.createRange();
        range.selectNodeContents(el);
        return (
          range.getBoundingClientRect().width >
          el.getBoundingClientRect().width + 0.5
        );
      })
      .map((el) => el.parentElement.id);
    return {
      backgrounds,
      wrongText,
      namedRegisters,
      terms,
      clippedAddresses,
      ambient: css(document.querySelector(".stage-light")).display,
    };
  });
  check(
    style.backgrounds.every(
      (s) => s.color === "rgb(0, 0, 0)" && s.image === "none",
    ),
    `${label}: pure black page/canvas, no decorative gradient`,
  );
  check(style.ambient === "none", `${label}: no ambient glow layer`);
  check(
    style.wrongText.length === 0,
    `${label}: ordinary text is pure white: ${style.wrongText.join(", ")}`,
  );
  check(
    style.namedRegisters && style.terms,
    `${label}: enlarged bold register names and readable terminology`,
  );
  check(
    style.clippedAddresses.length === 0,
    `${label}: full measured addresses fit their registers: ${style.clippedAddresses.join(", ")}`,
  );
}
async function signature(page, time) {
  await seek(page, time);
  return page.evaluate(() =>
    [
      ...document.querySelectorAll(
        ":is(.scene:not([hidden]),[data-film-continuity]) :is([data-anim],.reg-value,.page-value,.translation,.cache-byte,.frame-active,.array-value)",
      ),
    ].map((el) => {
      const s = getComputedStyle(el),
        r = el.getBoundingClientRect();
      return [
        el.id,
        s.opacity,
        s.visibility,
        s.transform,
        s.fill,
        s.stroke,
        s.color,
        el.textContent,
        ...[r.x, r.y, r.width, r.height].map((n) => n.toFixed(2)),
      ];
    }),
  );
}
async function textBounds(page) {
  return page.evaluate(() => {
    const stage = document.querySelector("#film-stage").getBoundingClientRect(),
      scale = stage.width / 1440;
    const scene = document.querySelector("#scene-host");
    const walker = document.createTreeWalker(scene, NodeFilter.SHOW_TEXT);
    const result = [];
    while (walker.nextNode()) {
      const text = walker.currentNode;
      if (!text.textContent.trim()) continue;
      let ok = true;
      for (let n = text.parentElement; n && n !== scene; n = n.parentElement) {
        const s = getComputedStyle(n);
        if (
          s.display === "none" ||
          s.visibility === "hidden" ||
          Number(s.opacity) < 0.2
        ) {
          ok = false;
          break;
        }
      }
      if (!ok || !text.parentElement.getClientRects().length) continue;
      const range = document.createRange();
      range.selectNodeContents(text);
      const r = range.getBoundingClientRect();
      const box = {
        x: (r.x - stage.x) / scale,
        y: (r.y - stage.y) / scale,
        right: (r.right - stage.x) / scale,
        bottom: (r.bottom - stage.y) / scale,
      };
      if (box.x < 15 || box.right > 1425 || box.y < 72 || box.bottom > 770)
        result.push({ text: text.textContent.trim().slice(0, 90), ...box });
    }
    return result;
  });
}
async function textOverlaps(page) {
  return page.evaluate(() => {
    const scene = document.querySelector("#scene-host");
    const nodes = [];
    let index = 0;
    const walker = document.createTreeWalker(scene, NodeFilter.SHOW_TEXT);
    while (walker.nextNode()) {
      const text = walker.currentNode;
      if (!text.textContent.trim()) continue;
      let visible = true;
      for (let n = text.parentElement; n && n !== scene; n = n.parentElement) {
        const style = getComputedStyle(n);
        if (
          style.display === "none" ||
          style.visibility === "hidden" ||
          Number(style.opacity) < 0.5
        ) {
          visible = false;
          break;
        }
      }
      if (!visible) continue;
      const range = document.createRange();
      range.selectNodeContents(text);
      for (const r of range.getClientRects())
        if (r.width > 1 && r.height > 1)
          nodes.push({
            index,
            text: text.textContent.trim().slice(0, 60),
            x: r.x,
            y: r.y,
            right: r.right,
            bottom: r.bottom,
          });
      index++;
    }
    const issues = [];
    for (let i = 0; i < nodes.length; i++)
      for (let j = i + 1; j < nodes.length; j++) {
        const a = nodes[i],
          b = nodes[j];
        if (a.index === b.index) continue;
        const w = Math.min(a.right, b.right) - Math.max(a.x, b.x),
          h = Math.min(a.bottom, b.bottom) - Math.max(a.y, b.y);
        if (w > 3 && h > 6 && w * h > 50) issues.push([a.text, b.text]);
      }
    return issues;
  });
}
async function contactSheet(pages) {
  const page = await browser.newPage({
    viewport: { width: 1850, height: 1000 },
  });
  for (const [id, shots] of pages) {
    const data = await Promise.all(
      shots.map(async (s) => ({
        ...s,
        image: (await readFile(s.path)).toString("base64"),
      })),
    );
    await page.setContent(
      '<html><body style="margin:0;background:#090b0e"><canvas id="sheet"></canvas></body></html>',
    );
    const size = await page.evaluate(async (items) => {
      const cellW = 900,
        cellH = 550,
        gap = 18;
      const canvas = document.querySelector("canvas");
      canvas.width = cellW * 2 + gap * 3;
      canvas.height = Math.ceil(items.length / 2) * (cellH + gap) + gap;
      const c = canvas.getContext("2d");
      c.fillStyle = "#090b0e";
      c.fillRect(0, 0, canvas.width, canvas.height);
      for (let i = 0; i < items.length; i++) {
        const x = gap + (i % 2) * (cellW + gap),
          y = gap + Math.floor(i / 2) * (cellH + gap);
        c.fillStyle = "#9db7cb";
        c.font = '17px "Noto Sans CJK SC"';
        c.fillText(items[i].label, x + 10, y + 25);
        const image = new Image();
        image.src = `data:image/png;base64,${items[i].image}`;
        await image.decode();
        c.drawImage(image, x, y + 39, cellW, (cellW * 9) / 16);
      }
      return { width: canvas.width, height: canvas.height };
    }, data);
    await page.setViewportSize(size);
    await page
      .locator("#sheet")
      .screenshot({ path: resolve(artifacts, `${id}-contact.png`) });
  }
  await page.close();
}
try {
  const sheets = new Map();
  for (const item of films) {
    const context = await browser.newContext({
      viewport: { width: 1600, height: 1160 },
    });
    const page = await context.newPage();
    monitor(page);
    await page.goto(
      `${origin}/chapter${item.chapter}/${item.file}.html?paused`,
    );
    await page.waitForFunction(() => window.film);
    const meta = await page.evaluate(() => ({
      duration: film.duration,
      chapters: film.chapters,
      steps: film.steps,
      checks: film.checks,
      experimentDuration: film.experimentDuration,
    }));
    check(
      (await page.evaluate(() => film.dataSource)) ===
        (item.dataKind || "teaching-model"),
      `${item.id}: measured/model boundary is explicit`,
    );
    check(
      meta.duration === item.duration,
      `${item.id}: catalog duration matches actual timeline`,
    );
    check(
      (await page.locator(".scene:not([hidden])").count()) === 1,
      `${item.id}: one initial scene`,
    );
    check(
      (await page.locator("#progress").getAttribute("max")) ===
        String(meta.duration),
      `${item.id}: true progress clock`,
    );
    for (const probe of meta.checks) {
      await seek(page, probe.time);
      if ("text" in probe)
        check(
          (await page.locator(probe.selector).textContent()).trim() ===
            probe.text,
          `${item.id} ${probe.time}: ${probe.selector} expected ${probe.text}`,
        );
      if ("shown" in probe)
        check(
          (await shown(page, probe.selector)) === probe.shown,
          `${item.id} ${probe.time}: ${probe.selector} visibility`,
        );
    }
    // Preserve the approved sample's exact staging during migration to the shared player.
    if (item.id === "stack-exhaustion")
      for (const [time, selector, visible] of [
        [0, "#call-frame", false],
        [3.18, "#call-frame", true],
        [13.35, "#capacity-limit", false],
        [14.775, "#capacity-limit", true],
        [18.6, "#overflow-error", false],
        [20.4, "#overflow-error", true],
        [23.91, "#return-caption", true],
        [28.65, "#return-summary", true],
      ]) {
        const point = time + meta.experimentDuration;
        await seek(page, point);
        check(
          (await shown(page, selector)) === visible,
          `sample ${selector} at ${point}`,
        );
      }
    const captures = [];
    for (const [i, chapter] of meta.chapters.entries()) {
      for (const t of [
        chapter.start,
        chapter.start + 0.05,
        chapter.start + chapter.duration - 0.001,
      ]) {
        await seek(page, t);
        check(
          (await page.locator(".scene:not([hidden])").count()) === 1,
          `${item.id}: no leakage at ${t}`,
        );
        check(
          (await clock(page)).chapter === i,
          `${item.id}: correct scene gate at ${t}`,
        );
      }
      const t = chapter.start + chapter.duration - 1;
      const forward = await signature(page, t);
      await seek(page, meta.duration);
      assert.deepEqual(
        await signature(page, t),
        forward,
        `${item.id}: reverse restores ${chapter.title}`,
      );
      assertions++;
      const bad = await textBounds(page);
      const overlaps = await textOverlaps(page);
      await presentation(page, `${item.id}/${chapter.id}`);
      if (chapter.id === "process-delayed-reap") {
        const unobscured = await page
          .locator("#delay-output")
          .evaluate((alias) => {
            const el = alias.matches('.carry-alias') ? alias.querySelector('[data-carry-actor]') : alias;
            const text = el.firstChild;
            return [...text.textContent].every((_, i) => {
              const range = document.createRange();
              range.setStart(text, i);
              range.setEnd(text, i + 1);
              const r = range.getBoundingClientRect();
              return [0.2, 0.5, 0.8].every((fraction) => {
                const hit = document.elementFromPoint(
                  r.left + r.width / 2,
                  r.top + r.height * fraction,
                );
                return hit && el.contains(hit);
              });
            });
          });
        check(unobscured, "large code must not cover the CP output glyphs");
      }
      check(
        overlaps.length === 0,
        `${item.id}/${chapter.id}: text overlap ${JSON.stringify(overlaps)}`,
      );
      check(
        bad.length === 0,
        `${item.id}/${chapter.id}: text fits safe stage bounds`,
      );
      if (bad.length)
        bounds.push({ film: item.id, scene: chapter.id, time: t, issues: bad });
      const path = resolve(
        artifacts,
        `${item.id}-${String(i + 1).padStart(2, "0")}.png`,
      );
      await page.locator(".film-viewport").screenshot({ path });
      captures.push({
        path,
        label: `${String(i + 1).padStart(2, "0")} · ${chapter.title} / ${t.toFixed(1)}s`,
      });
    }
    sheets.set(item.id, captures);
    for (const t of [
      meta.duration * 0.73,
      meta.duration * 0.12,
      meta.duration * 0.94,
      0,
    ]) {
      await seek(page, t);
      check(
        (await page.locator(".scene:not([hidden])").count()) === 1,
        `${item.id}: arbitrary seek gate`,
      );
    }
    await seek(page, 1);
    await page.getByRole("button", { name: "播放动画", exact: true }).click();
    await page.waitForTimeout(180);
    check(
      (await clock(page)).time > 1.07,
      `${item.id}: real playback advances`,
    );
    await page.getByRole("button", { name: "暂停动画", exact: true }).click();
    const frozen = (await clock(page)).time;
    await page.waitForTimeout(120);
    check(
      Math.abs((await clock(page)).time - frozen) < 0.001,
      `${item.id}: pause freezes`,
    );
    const stop = meta.steps[1];
    await seek(page, Math.max(0, stop - 0.25));
    await page.getByRole("button", { name: "下一步", exact: true }).click();
    await page.waitForFunction(() => !film.getState().playing);
    check(
      Math.abs((await clock(page)).time - stop) < 0.001,
      `${item.id}: next animates to a hold`,
    );
    await page.getByRole("button", { name: "上一步", exact: true }).click();
    check((await clock(page)).time === 0, `${item.id}: previous restores`);
    await page.getByRole("button", { name: "单步讲解", exact: true }).click();
    await seek(page, Math.max(0, stop - 0.2));
    await page.getByRole("button", { name: "播放动画", exact: true }).click();
    await page.waitForFunction(() => !film.getState().playing);
    check(
      Math.abs((await clock(page)).time - stop) < 0.001,
      `${item.id}: step mode`,
    );
    await page.getByRole("button", { name: "单步讲解", exact: true }).click();
    const track = await page.locator("#progress").boundingBox();
    await page.mouse.move(track.x + track.width * 0.3, track.y + 2);
    await page.mouse.down();
    await page.mouse.move(track.x + track.width * 0.65, track.y + 2, {
      steps: 5,
    });
    await page.mouse.up();
    check(
      Math.abs((await clock(page)).time - meta.duration * 0.65) < 0.4,
      `${item.id}: drag changes real teaching state`,
    );
    check(
      !(await clock(page)).playing,
      `${item.id}: paused drag remains paused`,
    );
    await page.evaluate(() => film.play());
    await page.mouse.move(track.x + track.width * 0.3, track.y + 2);
    await page.mouse.down();
    check(
      !(await clock(page)).playing,
      `${item.id}: active drag freezes playback`,
    );
    await page.mouse.move(track.x + track.width * 0.45, track.y + 2, {
      steps: 4,
    });
    await page.mouse.up();
    check(
      (await clock(page)).playing,
      `${item.id}: playing drag resumes playback`,
    );
    await page.evaluate(() => film.pause());
    await page.evaluate(() => document.activeElement?.blur());
    await page.keyboard.press("Space");
    check((await clock(page)).playing, `${item.id}: global Space plays`);
    await page.keyboard.press("Space");
    check(!(await clock(page)).playing, `${item.id}: global Space pauses`);
    await page.locator("#progress").focus();
    await page.keyboard.press("Home");
    check((await clock(page)).time === 0, `${item.id}: Home`);
    await page.keyboard.press("ArrowRight");
    check(
      (await clock(page)).time === 1,
      `${item.id}: seconds are real seconds`,
    );
    await page.keyboard.press("Shift+ArrowRight");
    check((await clock(page)).time === 6, `${item.id}: shift five seconds`);
    await page.keyboard.press("End");
    check(
      (await clock(page)).time === meta.duration,
      `${item.id}: final frame`,
    );
    await page.getByRole("button", { name: "重播", exact: true }).click();
    check(
      (await clock(page)).time < 0.2 && (await clock(page)).playing,
      `${item.id}: replay`,
    );
    await page.evaluate(() => film.pause());
    await page.getByRole("button", { name: "全屏观看", exact: true }).click();
    await page.waitForFunction(() => !!document.fullscreenElement);
    await page.evaluate(() => document.exitFullscreen());
    await context.close();
    const small = await browser.newContext({
      viewport: { width: 390, height: 844 },
      hasTouch: true,
      reducedMotion: "reduce",
    });
    const mobile = await small.newPage();
    monitor(mobile);
    await mobile.goto(`${origin}/chapter${item.chapter}/${item.file}.html`);
    await mobile.waitForFunction(() => window.film);
    check(
      !(await clock(mobile)).playing,
      `${item.id}: reduced motion defaults paused`,
    );
    check(
      await mobile.evaluate(
        () => document.documentElement.scrollWidth <= innerWidth,
      ),
      `${item.id}: no mobile horizontal overflow`,
    );
    const touch = await mobile.locator("#progress").boundingBox();
    await mobile.touchscreen.tap(touch.x + touch.width * 0.5, touch.y + 2);
    check(
      Math.abs((await clock(mobile)).time - meta.duration * 0.5) < 0.9,
      `${item.id}: touch seek`,
    );
    await small.close();
    console.log(
      `PASS: ${item.id} — ${meta.chapters.length} scenes, ${meta.checks.length} semantic checkpoints, reverse seek, transport, touch`,
    );
  }
  await writeFile(
    resolve(artifacts, "text-bounds.json"),
    JSON.stringify(bounds, null, 2),
  );
  await contactSheet(sheets);
  console.log(
    `Text-bound review: ${bounds.length} scenes flagged (film-artifacts/text-bounds.json)`,
  );

  if (!process.env.FILM_QUICK) {
    // Separate browser contexts keep each film visible. All registered films run at their real speed.
    await Promise.all(
      films.map(async (item) => {
        const context = await browser.newContext({
          viewport: { width: 1280, height: 980 },
          recordVideo: {
            dir: resolve(artifacts, "recordings"),
            size: { width: 1280, height: 980 },
          },
        });
        const page = await context.newPage();
        monitor(page);
        await page.goto(`${origin}/chapter${item.chapter}/${item.file}.html`);
        await page.waitForFunction(() => window.film);
        const seen = new Set(),
          positions = new Map();
        let motions = 0;
        const start = Date.now();
        while (Date.now() - start < (item.duration + 15) * 1000) {
          const current = await page.evaluate(() => {
            const samples = [
              ...document.querySelectorAll(
                ":is(.scene:not([hidden]),[data-film-continuity]) :is(.moving-packet,.packet)",
              ),
            ]
              .filter((el) => {
                const s = getComputedStyle(el);
                return s.visibility !== "hidden" && Number(s.opacity) > 0.5;
              })
              .map((el) => ({
                id: el.id,
                transform: el.getAttribute("transform"),
              }));
            return {
              ...film.getState(),
              samples,
              visible: document.querySelectorAll(".scene:not([hidden])").length,
            };
          });
          seen.add(current.chapter);
          check(
            current.visible === 1,
            `${item.id}: real playback has one scene`,
          );
          for (const sample of current.samples) {
            if (
              positions.has(sample.id) &&
              positions.get(sample.id) !== sample.transform
            )
              motions++;
            positions.set(sample.id, sample.transform);
          }
          if (current.time >= item.duration && !current.playing) break;
          await page.waitForTimeout(160);
        }
        const final = await clock(page);
        check(
          final.time === item.duration && !final.playing,
          `${item.id}: full real-time run completes`,
        );
        check(
          seen.size === (await page.evaluate(() => film.chapters.length)),
          `${item.id}: every scene actually played`,
        );
        check(
          Date.now() - start >= (item.duration - 1) * 1000,
          `${item.id}: not accelerated for acceptance`,
        );
        check(motions > 2, `${item.id}: packets truly move, not only flash`);
        const video = page.video();
        await context.close();
        await video.saveAs(resolve(artifacts, `${item.id}-preview.webm`));
        console.log(
          `PASS: ${item.id} — actual ${item.duration}s autoplay, ${motions} movement samples, recording saved`,
        );
      }),
    );
  }

  const production = createPreviewServer({
    directory: resolve(root, "dist"),
    prefix: "/csapp",
  });
  await new Promise((r) => production.listen(0, "127.0.0.1", r));
  const prodOrigin = `http://127.0.0.1:${production.address().port}`;
  try {
    const page = await browser.newPage();
    monitor(page, prodOrigin);
    await page.goto(`${prodOrigin}/csapp/`);
    check(
      (await page.locator(".chapter-card").count()) === catalogChapters.length,
      "published chapter index",
    );
    for (const item of films) {
      await page.goto(
        `${prodOrigin}/csapp/chapter${item.chapter}/${item.file}.html?paused`,
      );
      await page.waitForFunction(() => window.film);
      check(
        (await page.evaluate(() => film.duration)) === item.duration,
        `${item.id}: bundled film works in a project subpath`,
      );
      await seek(page, item.duration * 0.6);
      await presentation(page, `${item.id}: published contrast`);
      check(
        (await page.locator(".scene:not([hidden])").count()) === 1,
        `${item.id}: production state`,
      );
      await page.locator(".back-link").click();
      check(
        page.url().endsWith(`/chapter${item.chapter}/index.html`),
        `${item.id}: return to chapter`,
      );
      check(
        (await page.locator(".topic-card").count()) ===
          films.filter((f) => f.chapter === item.chapter).length,
        "chapter links",
      );
    }
    await page.close();
  } finally {
    await new Promise((r) => production.close(r));
  }
  check(errors.length === 0, `browser errors: ${errors.join("\n")}`);
  check(
    badResources.length === 0,
    `resource errors: ${badResources.join("\n")}`,
  );
  await writeFile(
    resolve(
      artifacts,
      process.env.FILM_QUICK ? "verification-quick.json" : "verification.json",
    ),
    JSON.stringify(
      {
        assertions,
        fullPlayback: !process.env.FILM_QUICK,
        films: films.map((f) => ({ id: f.id, duration: f.duration })),
        boundsFlagged: bounds.length,
        errors,
        badResources,
      },
      null,
      2,
    ),
  );
  console.log(
    `\n${assertions} browser assertions passed; ${process.env.FILM_QUICK ? "quick inspection" : "full real-time playback"}; production /csapp/ paths passed.`,
  );
} finally {
  await browser.close();
  await new Promise((r) => server.close(r));
}
