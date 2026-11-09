import { spawnSync } from "node:child_process";
import { readFile, writeFile, mkdir, copyFile, rename } from "node:fs/promises";
import { createHash } from "node:crypto";
import { resolve } from "node:path";
import { parseELF, elfSymbol, bytesAtVA } from "../models/elf.js";
import { validateOSBasics } from "../models/validate-os-basics.js";
const root = resolve(import.meta.dirname, ".."),
  work = ".trace-build/os-basics",
  compiler = process.env.CC || "gcc";
await mkdir(resolve(root, work), { recursive: true });
const flags = [
  "-std=c11",
  "-O0",
  "-g3",
  "-fPIE",
  "-fno-omit-frame-pointer",
  "-fno-optimize-sibling-calls",
  "-fno-ipa-stack-alignment",
  "-mno-red-zone",
  "-Wall",
  "-Wextra",
  "-Wpedantic",
];
const commands = [];
function run(executable, args, env = {}) {
  const result = spawnSync(executable, args, {
    cwd: root,
    encoding: "utf8",
    env: { ...process.env, ...env },
    timeout: 40000,
    maxBuffer: 20 * 1024 * 1024,
  });
  if (result.error || result.status !== 0)
    throw new Error(
      `${executable} failed: ${result.error || ""}\n${result.stdout}\n${result.stderr}`,
    );
  return result.stdout;
}
function build(args) {
  const command = [compiler, ...args].join(" ");
  commands.push(command);
  return run(compiler, args);
}
const sha = (data) => createHash("sha256").update(data).digest("hex");
for (const name of ["main", "math"]) {
  const file = `experiments/os-${name}.c`;
  build([...flags, "-E", file, "-o", `${work}/os-${name}.i`]);
  build([...flags, "-S", file, "-o", `${work}/os-${name}.s`]);
  build([...flags, "-c", file, "-o", `${work}/os-${name}.o`]);
}
build([
  "-pie",
  `${work}/os-main.o`,
  `${work}/os-math.o`,
  "-o",
  `${work}/os-demo`,
]);
const rawMain = await readFile(resolve(root, `${work}/os-main.o`)),
  rawMath = await readFile(resolve(root, `${work}/os-math.o`)),
  rawExecutable = await readFile(resolve(root, `${work}/os-demo`));
const elf = {
  main: parseELF(rawMain),
  math: parseELF(rawMath),
  executable: parseELF(rawExecutable),
};
await writeFile(
  resolve(root, `${work}/elf.json`),
  JSON.stringify(elf.executable),
);
const mainObject = elfSymbol(elf.main, "main"),
  mainLinked = elfSymbol(elf.executable, "main");
const rel = elf.main.relocations.find(
  (r) =>
    r.targetSection === ".text" &&
    r.type === 2 &&
    r.symbol === "os_data" &&
    BigInt(r.offset) >= BigInt(mainObject.value) &&
    BigInt(r.offset) < BigInt(mainObject.value) + BigInt(mainObject.size),
);
if (!rel)
  throw new Error(
    "Expected actual main PC32 reference is absent; no hand-made fallback",
  );
const P =
    BigInt(mainLinked.value) + BigInt(rel.offset) - BigInt(mainObject.value),
  S = elfSymbol(elf.executable, "os_data").value;
const text = elf.main.sections.find((s) => s.name === ".text");
const relocation = {
  type: rel.typeName,
  symbol: rel.symbol,
  S,
  A: rel.addend,
  P: `0x${P.toString(16).toUpperCase()}`,
  objectOffset: rel.offset,
  before: [
    ...rawMain.slice(
      text.offset + Number(BigInt(rel.offset)),
      text.offset + Number(BigInt(rel.offset)) + 4,
    ),
  ],
  after: bytesAtVA(rawExecutable, elf.executable, P, 4),
};
const sources = [];
for (const path of [
  "experiments/os-main.c",
  "experiments/os-math.c",
  "experiments/os-demo.h",
]) {
  const content = await readFile(resolve(root, path), "utf8");
  sources.push({ path, content, sha256: sha(content) });
}
const reports = [];
for (const [key, tool, args] of [
  ["object-header", "readelf", ["-h", `${work}/os-main.o`]],
  ["object-sections", "readelf", ["-SW", `${work}/os-main.o`]],
  ["object-symbols", "readelf", ["-sW", `${work}/os-main.o`]],
  ["math-symbols", "nm", ["-n", `${work}/os-math.o`]],
  ["object-relocations", "readelf", ["-rW", `${work}/os-main.o`]],
  ["math-disassembly", "objdump", ["-dr", `${work}/os-math.o`]],
  ["executable-header", "readelf", ["-h", `${work}/os-demo`]],
  ["executable-segments", "readelf", ["-lW", `${work}/os-demo`]],
  ["executable-disassembly", "objdump", ["-d", `${work}/os-demo`]],
])
  reports.push({
    key,
    command: [tool, ...args].join(" "),
    output: run(tool, args),
    path: `traces/os-basics/${key}.txt`,
  });
const runs = {},
  logs = [];
for (const name of ["first", "second", "fork"]) {
  run("gdb", ["-nx", "-q", "-batch", "-x", "scripts/gdb-os-basics.py"], {
    OS_RUN: name,
  });
  const value = JSON.parse(
    await readFile(resolve(root, `${work}/${name}.json`), "utf8"),
  );
  logs.push(`=== ${name} ===\n${value.commandsText}`);
  delete value.commandsText;
  runs[name] = value;
}
const trace = {
  schemaVersion: 1,
  id: "os-basics",
  provenance: {
    kind: "gdb-plus-model",
    capturedAt: new Date().toISOString(),
    sources,
    source: sources[0],
    binary: { path: `${work}/os-demo`, sha256: sha(rawExecutable) },
    compiler: run(compiler, ["--version"]).split("\n")[0],
    compilerCommand: commands.join(" ; "),
    gdb: `GNU gdb ${runs.first.gdb}`,
    binutils: run("readelf", ["--version"]).split("\n")[0],
    architecture: runs.first.architecture,
    kernelASLR: (
      await readFile("/proc/sys/kernel/randomize_va_space", "utf8")
    ).trim(),
    debuggerASLR: "set disable-randomization off (ASLR retained)",
    scope:
      "Actual ELF/tool/GDB/VA/value/ABI observations. Section/segment mapping, OS execution, CPU time-sharing and COW physical frames are explanatory, not privileged kernel/PFN or scheduler traces.",
  },
  build: { flags, commands, artifacts: [] },
  elf,
  relocation,
  reports,
  runs,
};
const preprocessed = (
  await readFile(resolve(root, `${work}/os-main.i`), "utf8")
).split("\n");
trace.build.preprocessed = preprocessed
  .map((line, i) => ({ line: i + 1, text: line.trim() }))
  .filter(
    (row) =>
      row.text.includes("os_result = os_add(") ||
      row.text.includes("os_initial_data = os_data"),
  );
const assembly = await readFile(resolve(root, `${work}/os-math.s`), "utf8");
const body = assembly.match(/^os_add:\s*\n([\s\S]*?)^\s*\.size\s+os_add,/m);
if (!body || trace.build.preprocessed.length !== 2)
  throw new Error("Missing actual preprocessor/assembler evidence");
trace.build.assemblyAdd = body[1]
  .split("\n")
  .map((line) => line.trim())
  .filter((line) => line && !line.startsWith(".") && !line.startsWith("#"));
const mathText = elf.math.sections.find((s) => s.name === ".text"),
  addSymbol = elfSymbol(elf.math, "os_add");
trace.build.addBytes = [
  ...rawMath.slice(
    mathText.offset + Number(BigInt(addSymbol.value)),
    mathText.offset + Number(BigInt(addSymbol.value)) + addSymbol.size,
  ),
];
const result = validateOSBasics(trace);
const publish = resolve(root, "traces/os-basics");
await mkdir(publish, { recursive: true });
for (const [name, target, kind] of [
  ["os-main.i", "os-main.i", "preprocessed"],
  ["os-math.i", "os-math.i", "preprocessed"],
  ["os-main.s", "os-main.s", "assembly"],
  ["os-math.s", "os-math.s", "assembly"],
  ["os-main.o", "os-main.o", "relocatable"],
  ["os-math.o", "os-math.o", "relocatable"],
  ["os-demo", "os-demo.elf", "executable-evidence"],
]) {
  const bytes = await readFile(resolve(root, work, name));
  await copyFile(resolve(root, work, name), resolve(publish, target));
  trace.build.artifacts.push({
    path: `traces/os-basics/${target}`,
    kind,
    size: bytes.length,
    sha256: sha(bytes),
  });
}
for (const report of reports)
  await writeFile(
    resolve(root, report.path),
    `$ ${report.command}\n${report.output}`,
  );
const final = resolve(root, "traces/os-basics.json");
await writeFile(final + ".tmp", JSON.stringify(trace, null, 2) + "\n");
await rename(final + ".tmp", final);
await writeFile(
  resolve(root, "traces/os-basics-gdb.txt"),
  `Captured ${trace.provenance.capturedAt}\nELF SHA256 ${trace.provenance.binary.sha256}\n${commands.join("\n")}\n\n${logs.join("\n\n")}`,
);
console.log(
  `PASS: OS basics: ${result.runs} actual runs / ${result.samples} GDB stops; 9 binutils reports; PC32 patch verified; complete C and file hashes published`,
);
