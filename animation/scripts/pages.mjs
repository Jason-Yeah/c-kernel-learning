import { writeFile, mkdir, readFile } from "node:fs/promises";
import { renderReading } from "./reading-page.mjs";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { chapters, films } from "../shared/catalog.js";
const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const head = (title, relative = "", extras = "") =>
  `<!doctype html>\n<html lang="zh-CN"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><meta name="theme-color" content="#000000"><title>${title} · CS / Motion</title><link rel="icon" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 30 30'%3E%3Crect width='30' height='30' rx='6' fill='%23090b0e'/%3E%3Cpath d='M7 7h16v4H7zm0 7h16v4H7zm0 7h16v2H7z' fill='%2369e2f4'/%3E%3C/svg%3E"><link rel="stylesheet" href="${relative}shared/film.css"><link rel="stylesheet" href="${relative}shared/diagrams.css"><link rel="stylesheet" href="${relative}shared/evidence.css">${extras}</head>`;
const footer = `<footer class="catalog-footer"><span>HTML + SVG + GSAP · 本地资源 · 无配音视觉讲解</span><span>独立分镜，逐步理解。</span></footer>`;
export async function generatePages() {
  await writeFile(
    resolve(root, "index.html"),
    `${head("计算机原理动画")}<body><main class="catalog-page"><header class="catalog-top"><a href="./index.html">CS / MOTION</a><span class="eyebrow">CS:APP · INTERACTIVE FILMS</span></header><section class="catalog-intro"><div class="eyebrow">EXPLORE BY CHAPTER</div><h1>计算机原理，看见过程。</h1><p>选一个章节，跟随调用、字节、地址和进程的运动，理解发生了什么。</p></section><nav class="chapter-grid" aria-label="章节目录">${chapters.map((c) => `<a class="chapter-card" href="./chapter${c.number}/index.html"><span class="chapter-number">CHAPTER ${String(c.number).padStart(2, "0")}</span><h2>${c.title}</h2><p>${c.description}</p><footer><span>${films.filter((f) => f.chapter === c.number).length} 部交互动画</span><span>进入章节 →</span></footer></a>`).join("")}</nav>${footer}</main></body></html>\n`,
  );
  for (const chapter of chapters) {
    const dir = resolve(root, `chapter${chapter.number}`);
    await mkdir(dir, { recursive: true });
    await writeFile(
      resolve(dir, "index.html"),
      `${head(chapter.title, "../")}<body><main class="catalog-page"><header class="catalog-top"><a href="../index.html">← CS / MOTION · 全部章节</a><span class="eyebrow">CHAPTER ${String(chapter.number).padStart(2, "0")}</span></header><section class="catalog-intro"><div class="eyebrow">第 ${chapter.number} 章</div><h1>${chapter.title}</h1><p>${chapter.description}</p>${chapter.number === 0 ? '<p><a href="./guide.html">阅读完整章节讲义 →</a> · 六节动画按下方顺序观看；每节附完整 C／ELF／GDB 实验台。</p>' : chapter.number === 2 ? '<p><a href="./guide.html">阅读整数与C位运算讲义 →</a> · 字节序 → 位运算 → 补码与转换；浮点数尚未开展。</p>' : chapter.number === 8 ? '<p><a href="./guide.html">阅读异常控制流基础讲义 →</a> · 8-1为本轮新片；生命周期与父子内存已有独立动画。</p>' : ""}</section><nav class="topic-list" aria-label="本章动画">${films
        .filter((f) => f.chapter === chapter.number)
        .map(
          (f) =>
            `<a class="topic-card" href="./${f.file}.html"><span class="topic-number">${f.number}</span><div><h2>${f.title}</h2><p>${f.summary}</p></div><span class="topic-duration">${f.duration}s / 逐步讲解</span><span class="topic-arrow">↗</span></a>`,
        )
        .join("")}</nav>${footer}</main></body></html>\n`,
    );
  }
  const manuscript = await readFile(
    resolve(root, "chapter0/os-basics.md"),
    "utf8",
  );
  await writeFile(
    resolve(root, "chapter0/guide.html"),
    `${head("OS 与程序运行基础 · 完整讲义", "../", '<link rel="stylesheet" href="../shared/os-basics.css">')}<body><main class="os-reading-page"><nav><a href="./index.html">← 独立基础章 · 六节动画</a> · <a href="./os-basics.md">Markdown 原稿 ↗</a></nav><article>${renderReading(manuscript)}</article><footer><a href="./index.html">返回本章动画目录 →</a></footer></main></body></html>\n`,
  );
  const integerManuscript = await readFile(resolve(root, "chapter2/integers.md"), "utf8");
  await writeFile(resolve(root, "chapter2/guide.html"), `${head("整数与C位运算 · 讲义", "../", '<link rel="stylesheet" href="../shared/os-basics.css"><link rel="stylesheet" href="../shared/integer-bits.css">')}<body><main class="os-reading-page"><nav><a href="./index.html">← 第2章 · 信息的表示与处理</a> · <a href="./integers.md">Markdown 原稿 ↗</a></nav><article>${renderReading(integerManuscript)}</article><footer><a href="./index.html">返回本章动画目录 →</a></footer></main></body></html>\n`);
  const controlManuscript = await readFile(resolve(root, "chapter8/exception-control.md"), "utf8");
  await writeFile(resolve(root, "chapter8/guide.html"), `${head("异常控制流基础 · 讲义", "../", '<link rel="stylesheet" href="../shared/os-basics.css">')}<body><main class="os-reading-page"><nav><a href="./index.html">← 第8章 · 异常控制流</a> · <a href="./exception-control.md">Markdown原稿 ↗</a></nav><article>${renderReading(controlManuscript)}</article><footer><a href="./index.html">返回第8章动画 →</a></footer></main></body></html>\n`);
  for (const film of films)
    await writeFile(
      resolve(root, `chapter${film.chapter}/${film.file}.html`),
      `${head(film.title, "../", [0, 1, 2].includes(film.chapter) ? `<link rel="stylesheet" href="../shared/os-basics.css">${film.chapter === 1 ? '<link rel="stylesheet" href="../shared/instruction-cycle.css">' : film.chapter === 2 ? `<link rel="stylesheet" href="../shared/${film.id === 'byte-order' ? 'byte-order' : 'integer-bits'}.css">` : ""}` : film.id === 'exception-control' ? '<link rel="stylesheet" href="../shared/os-basics.css"><link rel="stylesheet" href="../shared/control-flow.css">' : "")}<body><div id="app"><p class="boot-message">正在载入动画…</p></div><noscript><p class="no-script">此交互动画需要启用 JavaScript。</p></noscript><script type="module" src="./${film.file}.js"></script></body></html>\n`,
    );
}
if (
  process.argv[1] &&
  resolve(process.argv[1]) === fileURLToPath(import.meta.url)
) {
  await generatePages();
  console.log(
    `Generated ${chapters.length} chapter indexes and ${films.length} film entries`,
  );
}
