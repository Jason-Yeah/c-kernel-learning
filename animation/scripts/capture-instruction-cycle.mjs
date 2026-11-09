import { spawnSync } from "node:child_process";
import { readFile, writeFile, mkdir, rename } from "node:fs/promises";
import { createHash } from "node:crypto";
import { resolve } from "node:path";
import { parseELF, elfSymbol, bytesAtVA } from "../models/elf.js";
import {
  identifyCycleEvents,
  validateInstructionCycle,
} from "../models/validate-instruction-cycle.js";
const root = resolve(import.meta.dirname, ".."),
  dir = ".trace-build/instruction-cycle";
const flags = [
  "-std=c11",
  "-O0",
  "-g3",
  "-fPIE",
  "-pie",
  "-fno-omit-frame-pointer",
  "-fno-optimize-sibling-calls",
  "-fno-ipa-stack-alignment",
  "-mno-red-zone",
  "-Wall",
  "-Wextra",
  "-Wpedantic",
];
const compiler = process.env.CC || "gcc",
  sourcePath = "experiments/instruction-cycle.c",
  binaryPath = dir + "/program";
function command(exe, args) {
  const p = spawnSync(exe, args, {
    cwd: root,
    encoding: "utf8",
    timeout: 35000,
    maxBuffer: 16 * 1024 * 1024,
  });
  if (p.error || p.status !== 0)
    throw new Error(
      `${exe} failed; failing capture not published\n${p.error || ""}\n${p.stdout}\n${p.stderr}`,
    );
  return p.stdout;
}
const sha = (data) => createHash("sha256").update(data).digest("hex");
await mkdir(resolve(root, dir), { recursive: true });
command(compiler, [...flags, sourcePath, "-o", binaryPath]);
const rawLog = command("gdb", [
  "-nx",
  "-q",
  "-batch",
  "-x",
  "scripts/gdb-instruction-cycle.py",
]);
await writeFile(resolve(root, dir, "session.log"), rawLog);
const raw = JSON.parse(await readFile(resolve(root, dir, "raw.json"), "utf8"));
const source = await readFile(resolve(root, sourcePath), "utf8"),
  binary = await readFile(resolve(root, binaryPath));
const elf = parseELF(binary),
  functionSymbol = elfSymbol(elf, "calculate");
const loadBias = BigInt(raw.function.address) - BigInt(functionSymbol.value);
for (const sample of raw.samples.filter(
  (s) => s.source.function === "calculate",
)) {
  const expected = bytesAtVA(
    binary,
    elf,
    BigInt(sample.registers.rip) - loadBias,
    sample.instruction.length,
  );
  if (expected.join(",") !== sample.instruction.bytes.join(","))
    throw new Error("GDB code bytes do not match actual ELF");
}
const reports = [
  {
    key: "disassembly",
    command: `objdump -d ${binaryPath}`,
    output: command("objdump", ["-d", binaryPath]),
  },
  {
    key: "elf",
    command: `readelf -h ${binaryPath}`,
    output: command("readelf", ["-h", binaryPath]),
  },
];
const trace = {
  schemaVersion: 1,
  id: "instruction-cycle",
  provenance: {
    kind: "gdb-plus-model",
    capturedAt: new Date().toISOString(),
    source: { path: sourcePath, content: source, sha256: sha(source) },
    binary: {
      path: "traces/instruction-cycle/program.elf",
      sha256: sha(binary),
    },
    compiler: command(compiler, ["--version"]).split("\n")[0],
    compilerCommand: [compiler, ...flags, sourcePath, "-o", binaryPath].join(
      " ",
    ),
    flags,
    gdb: `GNU gdb ${raw.gdb}`,
    architecture: raw.architecture,
    kernelASLR: (
      await readFile("/proc/sys/kernel/randomize_va_space", "utf8")
    ).trim(),
    debuggerASLR: raw.randomization,
    scope:
      "Actual x86-64 instruction boundaries, bytes, registers and memory. Fetch/decode/ALU/control arrows are explanatory; no hardware cycles, decoder/pipeline/Cache/TLB or speculative execution measured.",
  },
  loadBias: "0x" + loadBias.toString(16).toUpperCase(),
  elfSymbols: {
    calculate: functionSymbol,
    inputs: elfSymbol(elf, "inputs"),
    observed: elfSymbol(elf, "observed"),
  },
  function: raw.function,
  returnAddress: raw.returnAddress,
  events: identifyCycleEvents(raw.samples),
  samples: raw.samples,
  exitCode: raw.exitCode,
  stdout: raw.stdout,
  reports,
};
const result = validateInstructionCycle(trace);
await mkdir(resolve(root, "traces/instruction-cycle"), { recursive: true });
await writeFile(resolve(root, "traces/instruction-cycle/program.elf"), binary);
for (const r of reports)
  await writeFile(
    resolve(root, `traces/instruction-cycle/${r.key}.txt`),
    `$ ${r.command}\n${r.output}`,
  );
const target = resolve(root, "traces/instruction-cycle.json");
await writeFile(target + ".tmp", JSON.stringify(trace, null, 2) + "\n");
await rename(target + ".tmp", target);
await writeFile(
  resolve(root, "traces/instruction-cycle-gdb.txt"),
  `Captured: ${trace.provenance.capturedAt}\nSource SHA256: ${trace.provenance.source.sha256}\nELF SHA256: ${trace.provenance.binary.sha256}\n${trace.provenance.compilerCommand}\n\n${raw.gdbText}`,
);
console.log(
  `PASS: instruction cycle: ${result.samples} real stops / ${result.events} events; actual ELF bytes, MOV EA, ADD, STORE and RET verified`,
);
