import { spawnSync } from "node:child_process";
import { readFile, writeFile, mkdir, rename } from "node:fs/promises";
import { createHash } from "node:crypto";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { release } from "node:os";
import { validateStackTrace } from "../models/validate-stack-trace.js";
const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const compiler = process.env.CC || "gcc";
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
const sourcePath = "experiments/stack-call.c";
const binaryPath = ".trace-build/stack-call";
const sha256 = (data) => createHash("sha256").update(data).digest("hex");
function command(executable, args, timeout = 20000) {
  const run = spawnSync(executable, args, {
    cwd: root,
    encoding: "utf8",
    timeout,
    maxBuffer: 8 * 1024 * 1024,
  });
  if (run.error || run.status !== 0)
    throw new Error(
      `${executable} failed; no measured data published.\n${run.error || ""}\n${run.stdout}\n${run.stderr}`,
    );
  return run.stdout;
}
await mkdir(resolve(root, ".trace-build"), { recursive: true });
await mkdir(resolve(root, "traces"), { recursive: true });
command(compiler, [...flags, sourcePath, "-o", binaryPath]);
const output = command(
  "gdb",
  ["-nx", "-q", "-batch", "-x", "scripts/gdb-stack.py"],
  30000,
);
await writeFile(resolve(root, ".trace-build/gdb-session.log"), output);
const raw = JSON.parse(
  await readFile(resolve(root, ".trace-build/stack-raw.json"), "utf8"),
);
const source = await readFile(resolve(root, sourcePath), "utf8");
const binary = await readFile(resolve(root, binaryPath));
const trace = {
  schemaVersion: 1,
  provenance: {
    kind: "gdb-measured",
    capturedAt: new Date().toISOString(),
    source: { path: sourcePath, sha256: sha256(source), content: source },
    binary: { path: binaryPath, sha256: sha256(binary) },
    compiler: command(compiler, ["--version"]).split("\n")[0],
    compilerCommand: [compiler, ...flags, sourcePath, "-o", binaryPath].join(
      " ",
    ),
    flags,
    gdb: `GNU gdb ${raw.gdbVersion}`,
    kernel: release(),
    kernelASLR: (
      await readFile("/proc/sys/kernel/randomize_va_space", "utf8")
    ).trim(),
    debuggerASLR: raw.randomization,
    replay: "Offline replay of one capture; this website does not run GDB.",
    variableCaution:
      "Variables seen in scope may be uninitialized at entry. Animation uses register arguments before the prologue, and only verified locals after initialization.",
    geometryCaution:
      "Addresses and values are measured. Layout, arrows and teaching timing are designed, not a recording of CPU timing.",
  },
  architecture: raw.architecture,
  endian: raw.endian,
  exitCode: raw.exitCode,
  functions: raw.functions,
  events: raw.events,
  samples: raw.samples,
};
const summary = validateStackTrace(trace);
const target = resolve(root, "traces/stack-call.json");
await writeFile(target + ".tmp", JSON.stringify(trace, null, 2) + "\n");
await rename(target + ".tmp", target);
await writeFile(
  resolve(root, "traces/stack-call-gdb.txt"),
  `Captured: ${trace.provenance.capturedAt}\nSource SHA256: ${trace.provenance.source.sha256}\nELF SHA256: ${trace.provenance.binary.sha256}\nCompile: ${trace.provenance.compilerCommand}\n\n${raw.gdbText}`,
);
console.log(
  `PASS: ${summary.samples} real instruction stops, ${summary.events} events; source/ELF hashes recorded; traces/stack-call.json updated`,
);
