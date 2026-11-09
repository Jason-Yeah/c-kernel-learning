// Trace-independent vertical memory/file array, shared by OS and later chapters.
import { pos, escape, reveal } from "./scene-kit.js";
export function memory(
  id,
  x,
  y,
  rows,
  {
    width = 480,
    title = "进程虚拟地址空间",
    direction = "高地址在上 · VA ↑ 增大 · 空洞已折叠",
    cellHeight = 56,
    file = false,
    staged = false,
    space = "virtual",
  } = {},
) {
  return pos(
    id,
    x,
    y,
    `<div class="array-header"><strong>${escape(title)}</strong><span>${escape(direction)}</span></div>${rows.map((r, i) => `<div id="${id}-${r.key || i}" class="array-row ${r.role || ""}" ${staged ? "data-anim" : ""} data-address="${escape(r.address || "")}" data-memory-key="${escape(r.key || i)}"><span class="array-ident"><span class="array-name">${escape(r.label)}</span><code class="array-address">${escape(r.address || r.detail || "")}</code></span><b class="array-value">${escape(r.value ?? "")}</b></div>`).join("")}`,
    `vertical-array ${file ? "file-array" : space + "-memory"}`,
    { width },
  )
    .replace(
      `width:${width}px;`,
      `width:${width}px;--cell-height:${cellHeight}px;`,
    )
    .replace(
      'class="positioned',
      `data-space="${file ? "file" : space}" class="positioned`,
    );
}
export function showMemory(
  s,
  id,
  at,
  { staged = false, interval = 0.45 } = {},
) {
  reveal(s.timeline, s.$("#" + id), at, { y: 0, duration: 0.55 });
  if (staged)
    s.$$(`#${id} .array-row`).forEach((row, i) =>
      reveal(s.timeline, row, at + 0.25 + i * interval, {
        y: 12,
        duration: 0.5,
      }),
    );
}
