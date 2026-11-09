import { spawnSync } from "node:child_process";
import { readFile, writeFile, mkdir, rename } from "node:fs/promises";
import { createHash } from "node:crypto";
import { resolve } from "node:path";
import { release } from "node:os";
import { validateExperimentTrace } from "../models/validate-experiments.js";

const root = resolve(import.meta.dirname, "..");
const compiler = process.env.CC || "gcc";
const experiments = {
  "buffer-copy": {
    cases: ["safe", "edge", "broken"],
    kind: "gdb-measured",
    limits:
      "Controlled local UB only; no shellcode. Actual compiler layout and actual SIGSEGV, not a portable C guarantee.",
  },
  "stack-recursion": {
    cases: ["exhaust", "bounded"],
    kind: "gdb-plus-model",
    limits:
      "Actual depth/register/fault observations. Eight drawn frames are a reading aid, not the measured maximum; normal return is a separate run.",
  },
  "cache-access": {
    cases: ["default"],
    kind: "gdb-plus-model",
    limits:
      "Actual C addresses, bytes and loads. GDB perturbs Cache and cannot observe hardware tags/LRU/HIT; the four-set two-way eight-byte Cache is an explicit mechanism model.",
  },
  "process-lifecycle": {
    cases: ["early", "late"],
    kind: "gdb-measured",
    limits:
      "Two pipe-constrained executions. A before-wait breakpoint is not evidence that CPU sleep or every scheduling interleaving was observed.",
  },
  "fork-memory": {
    cases: ["default"],
    kind: "gdb-plus-model",
    limits:
      "VA, values and mapping permissions observed. No PFNs/physical copying/PTE internals inferred from matching VAs or maps; COW internals remain a mechanism model.",
  },
  "vm-access": {
    cases: ["resident-a", "resident-b", "cold", "readonly", "unmapped"],
    kind: "gdb-plus-model",
    limits:
      "Actual mmap/permissions/present flags/read values/signals. No hardware TLB or PFN inferred; a first anonymous read may use Linux's shared zero page, not a newly allocated private frame. Collector never dereferences the cold target before access; minor-fault deltas can include debugger effects.",
  },
};
const selected = process.argv.slice(2);
for (const id of selected)
  if (!experiments[id]) throw new Error(`Unknown reviewed experiment: ${id}`);
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
  "-fno-stack-protector",
  "-U_FORTIFY_SOURCE",
  "-D_FORTIFY_SOURCE=0",
  "-Wall",
  "-Wextra",
  "-Wpedantic",
];
const sha256 = (data) => createHash("sha256").update(data).digest("hex");
function command(executable, args, env = {}, timeout = 30000) {
  const run = spawnSync(executable, args, {
    cwd: root,
    env: { ...process.env, ...env },
    encoding: "utf8",
    timeout,
    maxBuffer: 16 * 1024 * 1024,
  });
  if (run.error || run.status !== 0)
    throw new Error(
      `${executable} failed; no evidence published.\n${run.error || ""}\n${run.stdout}\n${run.stderr}`,
    );
  return run.stdout;
}
await mkdir(resolve(root, ".trace-build"), { recursive: true });
await mkdir(resolve(root, "traces"), { recursive: true });
for (const [id, spec] of Object.entries(experiments)) {
  if (selected.length && !selected.includes(id)) continue;
  const sourcePath = `experiments/${id}.c`,
    binaryPath = `.trace-build/${id}`;
  command(compiler, [...flags, sourcePath, "-o", binaryPath]);
  const runs = {},
    raw = [];
  let architecture, debuggerASLR, gdbVersion;
  for (const name of spec.cases) {
    const output = command(
      "gdb",
      ["-nx", "-q", "-batch", "-x", "scripts/gdb-experiments.py"],
      { ANIMATION_EXPERIMENT: id, ANIMATION_CASE: name },
    );
    await writeFile(resolve(root, `.trace-build/${id}-${name}.log`), output);
    const run = JSON.parse(
      await readFile(resolve(root, `.trace-build/${id}-${name}.json`), "utf8"),
    );
    ({ architecture, randomization: debuggerASLR, gdbVersion } = run);
    raw.push(`=== RUN ${name} ===\n${run.gdbText}`);
    delete run.gdbText;
    delete run.gdbVersion;
    delete run.randomization;
    runs[name] = run;
  }
  const source = await readFile(resolve(root, sourcePath), "utf8");
  const trace = {
    schemaVersion: 1,
    id,
    provenance: {
      kind: spec.kind,
      capturedAt: new Date().toISOString(),
      source: { path: sourcePath, content: source, sha256: sha256(source) },
      binary: {
        path: binaryPath,
        sha256: sha256(await readFile(resolve(root, binaryPath))),
      },
      compiler: command(compiler, ["--version"]).split("\n")[0],
      compilerCommand: [compiler, ...flags, sourcePath, "-o", binaryPath].join(
        " ",
      ),
      flags,
      gdb: `GNU gdb ${gdbVersion}`,
      architecture,
      kernel: release(),
      kernelASLR: (
        await readFile("/proc/sys/kernel/randomize_va_space", "utf8")
      ).trim(),
      debuggerASLR,
      limits: spec.limits,
      replay:
        "Offline replay of actual local GDB stops; the website does not execute programs or control GDB.",
    },
    runs,
  };
  const summary = validateExperimentTrace(trace);
  const target = resolve(root, `traces/${id}.json`);
  await writeFile(target + ".tmp", JSON.stringify(trace, null, 2) + "\n");
  await rename(target + ".tmp", target);
  await writeFile(
    resolve(root, `traces/${id}-gdb.txt`),
    `Captured: ${trace.provenance.capturedAt}\nSource SHA256: ${trace.provenance.source.sha256}\nELF SHA256: ${trace.provenance.binary.sha256}\nCompile: ${trace.provenance.compilerCommand}\nScope: ${spec.limits}\n\n${raw.join("\n")}`,
  );
  console.log(
    `PASS: ${id}: ${summary.runs} actual runs, ${summary.samples} GDB stops, hashes and commands published`,
  );
}
