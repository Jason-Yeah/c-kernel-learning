import trace from "../traces/os-basics.json" with { type: "json" };
import { validateOSBasics, osSample, osObject } from "./validate-os-basics.js";
export const osTrace = trace;
export const osSummary = validateOSBasics(trace);
export const hexAddress = (value) =>
  `0x${BigInt(value).toString(16).toUpperCase()}`;
export const byteHex = (n) =>
  Number(n).toString(16).toUpperCase().padStart(2, "0");
export const sample = (event, run = "first") => osSample(trace, event, run);
export const object = osObject;
export const ready = sample("layout-ready");
export const computed = sample("compute-done");
export const entry = sample("app-entry");
export const interpreter = sample("interpreter-entry");
export const mainEntry = sample("main-entry");
export const secondEntry = sample("app-entry", "second");
export const parent = sample("phase-2", "fork");
export const child = sample("phase-3", "fork");
export const changed = sample("phase-4", "fork");
export const parentAfter = sample("phase-5", "fork");
export const addEntry = sample("os_add-entry");
export const addReturned = sample("os_add-returned");
export const widthBefore = trace.runs.first.samples.find(
  (s) => s.event.startsWith("os_width") && /\$0x5,%eax/.test(s.instruction.asm),
);
export const widthAfter = trace.runs.first.samples.find(
  (s) =>
    s.event.startsWith("os_width") &&
    /\$0x5,%eax/.test(s.previousInstruction?.asm || ""),
);
export const compared = trace.runs.first.samples.find(
  (s) =>
    s.event.startsWith("os_equal") &&
    s.previousInstruction?.asm.startsWith("cmp"),
);
export const flagBits = Object.fromEntries(
  [
    ["CF", 0],
    ["ZF", 6],
    ["SF", 7],
    ["OF", 11],
  ].map(([name, bit]) => [
    name,
    Number((BigInt(compared.registers.eflags) >> BigInt(bit)) & 1n),
  ]),
);
export const loadBias = hexAddress(
  BigInt(entry.registers.rip) - BigInt(trace.elf.executable.header.entry),
);
export const secondBias = hexAddress(
  BigInt(secondEntry.registers.rip) - BigInt(trace.elf.executable.header.entry),
);
export const loadSegments = trace.elf.executable.programs.filter(
  (p) => p.type === 1,
);
export const interpreterPath = trace.elf.executable.programs.find(
  (p) => p.type === 3,
).interpreter;
export const section = (which, name) => {
  const s = trace.elf[which].sections.find((s) => s.name === name);
  if (!s) throw new Error(`Missing observed section ${which}/${name}`);
  return s;
};
export const symbol = (which, name) => {
  const s = trace.elf[which].symbols.find(
    (s) => s.table === ".symtab" && s.name === name,
  );
  if (!s) throw new Error(`Missing observed symbol ${which}/${name}`);
  return s;
};
export const report = (key) => {
  const r = trace.reports.find((r) => r.key === key);
  if (!r) throw new Error("Missing actual report " + key);
  return r;
};
export const source = (name) => {
  const s = trace.provenance.sources.find((s) => s.path.endsWith("/" + name));
  if (!s) throw new Error("Missing full program " + name);
  return s;
};
export function sourceRows(name, needles) {
  const lines = source(name).content.split("\n");
  const numbers = needles.map((needle) => {
    const matches = lines
      .map((s, i) => (s.includes(needle) ? i + 1 : null))
      .filter(Boolean);
    if (matches.length !== 1)
      throw new Error(`Ambiguous/missing actual source ${name}: ${needle}`);
    return matches[0];
  });
  return { lines: numbers.map((n) => lines[n - 1].trim()), numbers };
}
export function sortedObjects(state = ready, names = null) {
  return state.objects
    .filter((o) => !names || names.includes(o.name))
    .slice()
    .sort((a, b) => (BigInt(a.address) > BigInt(b.address) ? -1 : 1));
}
export function mapping(address, state = ready) {
  const at = BigInt(address),
    line = state.maps.find((line) => {
      const [a, b] = line
        .split(" ")[0]
        .split("-")
        .map((s) => BigInt("0x" + s));
      return at >= a && at < b;
    });
  if (!line) throw new Error("No observed mapping for " + address);
  const [range, permission] = line.split(/\s+/),
    [start, end] = range.split("-").map((s) => hexAddress("0x" + s));
  return { start, end, permission, line };
}
export const memoryLabels = {
  text: ".text · 程序代码",
  rodata: ".rodata · 常量",
  data: ".data · 初值全局",
  bss: ".bss · 零初值全局",
  heap: "malloc · 动态对象",
  private: "mmap · PRIVATE",
  shared: "mmap · SHARED",
  stack: "stack · 局部变量",
};
