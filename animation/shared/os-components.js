import { pos, escape, code, number } from "./scene-kit.js";
export { memory, showMemory } from "./vertical-memory.js";
import {
  sourceRows,
  osTrace,
  osSummary,
  sortedObjects,
  memoryLabels,
} from "../models/os-basics.js";
import { mountFilm } from "./player.js";
import { attachOSLab } from "./os-lab.js";
export function memoryRows(state, names = null, { values = true } = {}) {
  return sortedObjects(state, names).map((o) => ({
    key: o.name,
    label: memoryLabels[o.name],
    address: o.address,
    value: values && o.value !== undefined ? o.value : "",
    role: o.name === "shared" ? "target-row" : "",
  }));
}
export function file(id, x, y, title, description, { width = 260 } = {}) {
  return pos(
    id,
    x,
    y,
    `<svg viewBox="0 0 260 190" preserveAspectRatio="none" aria-hidden="true"><path d="M1 1H231L259 29V189H1Z M231 1V29H259"/></svg><strong>${escape(title)}</strong><span>${description}</span>`,
    "os-file",
    { width },
  );
}
export function actualCode(id, x, y, name, needles, { width = 600 } = {}) {
  const data = sourceRows(name, needles);
  return code(id, x, y, data.lines, {
    width,
    title: `${name} / 真实源句节选`,
    lineNumbers: data.numbers,
  });
}
export function commit(s, selector, value, at) {
  number(s.timeline, s.$(selector), String(value), at);
}
export function osShot(s) {
  s.root.classList.add("os-scene");
  return s;
}
export function mountOS(config) {
  const p = osTrace.provenance;
  const api = mountFilm({
    ...config,
    chapter: 0,
    model:
      "C／ELF／GDB 离线实测 + 机制解释 · 竖向内存高地址在上 · 非物理 RAM 比例图",
    example: "../experiments/os-main.c",
    provenance: {
      ...p,
      kind: "gdb-plus-model",
      source: { path: p.source.path, sha256: p.source.sha256 },
      samples: osSummary.samples,
      events: osSummary.samples,
      sourceHref: "../experiments/os-main.c",
      jsonHref: "../traces/os-basics.json",
      gdbHref: "../traces/os-basics-gdb.txt",
    },
  });
  attachOSLab(api, config.id, config.labLocations || {});
  return api;
}
