import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { createHash } from "node:crypto";
import { resolve } from "node:path";
import {
  validateExperimentTrace,
  phaseOf,
} from "../models/validate-experiments.js";
const root = resolve(import.meta.dirname, "..");
const ids = [
  "buffer-copy",
  "stack-recursion",
  "cache-access",
  "process-lifecycle",
  "fork-memory",
  "vm-access",
];
const traces = Object.fromEntries(
  await Promise.all(
    ids.map(async (id) => [
      id,
      JSON.parse(await readFile(resolve(root, `traces/${id}.json`), "utf8")),
    ]),
  ),
);
for (const [id, trace] of Object.entries(traces))
  test(`actual C/GDB experiment: ${id}`, async () => {
    assert.ok(validateExperimentTrace(trace).samples > 0);
    const source = await readFile(
      resolve(root, trace.provenance.source.path),
      "utf8",
    );
    assert.equal(source, trace.provenance.source.content);
    assert.equal(
      createHash("sha256").update(source).digest("hex"),
      trace.provenance.source.sha256,
    );
    assert.ok(
      trace.provenance.compilerCommand.includes("-O0") &&
        trace.provenance.compilerCommand.includes("-g3"),
    );
    assert.ok(trace.provenance.debuggerASLR.includes("off"));
    const text = await readFile(resolve(root, `traces/${id}-gdb.txt`), "utf8");
    assert.ok(text.includes(trace.provenance.source.sha256));
    for (const run of Object.values(trace.runs))
      for (const sample of run.samples) {
        const registers = sample.commands.find((c) =>
          c.command.startsWith("info registers"),
        );
        const rax = registers.output.match(/rax\s+(0x[0-9a-f]+)/i)?.[1];
        assert.equal(
          BigInt(sample.registers.rax),
          BigInt(rax),
          "64-bit register bits agree with original GDB, including signed displays",
        );
        assert.ok(sample.commands.some((c) => c.command === "x/i $rip"));
      }
  });
test("cold-page collector did not fault in target memory before the C access", () => {
  const run = traces["vm-access"].runs.cold,
    before = phaseOf(run, 1),
    after = phaseOf(run, 2);
  assert.equal(before.fields.page.present, false);
  assert.equal(before.fields.byte, undefined);
  assert.ok(
    before.commands.every(
      (c) => !/^x\/(?!i)/.test(c.command) && !/^p\s+\*target/.test(c.command),
    ),
  );
  assert.equal(after.fields.page.present, true);
  assert.equal(after.fields.byte, 0);
});
test("real return target keeps observed high bytes, not a fabricated 0xDEAD", () => {
  const run = traces["buffer-copy"].runs.broken,
    done = phaseOf(run, 3);
  assert.notEqual(BigInt(done.fields.savedReturn), 0xdeadn);
  assert.equal(BigInt(done.fields.savedReturn) & 0xffffffn, 0xdeadn);
  const fault = run.samples.find((s) => s.event === "signal");
  assert.equal(fault.signal.address, done.fields.savedReturn);
});
test("missing or forged evidence is rejected rather than replaced by teaching constants", () => {
  const buffer = structuredClone(traces["buffer-copy"]);
  buffer.runs.broken.samples.find((s) => s.phase === 2).fields.memory[0] ^= 1;
  assert.throws(() => validateExperimentTrace(buffer));
  const process = structuredClone(traces["process-lifecycle"]);
  process.runs.early.samples = process.runs.early.samples.filter(
    (s) => s.event !== "exec",
  );
  assert.throws(() => validateExperimentTrace(process));
  const vm = structuredClone(traces["vm-access"]);
  phaseOf(vm.runs.cold, 1).fields.page.present = true;
  assert.throws(() => validateExperimentTrace(vm));
});
