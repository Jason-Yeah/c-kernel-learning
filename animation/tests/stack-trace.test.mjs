import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { createHash } from "node:crypto";
import trace from "../traces/stack-call.json" with { type: "json" };
import { validateStackTrace, eventOf } from "../models/validate-stack-trace.js";

test("GDB capture: real call/ret transitions and initialized variable invariants", () => {
  const summary = validateStackTrace(trace);
  assert.equal(summary.events, 12);
  assert.ok(summary.samples >= 20);
  assert.equal(trace.provenance.kind, "gdb-measured");
  assert.match(trace.provenance.compilerCommand, /-O0/);
  assert.match(trace.provenance.compilerCommand, /-mno-red-zone/);
  assert.match(trace.provenance.debuggerASLR, /is off/);
});
test("GDB capture: source SHA and all published stack word bytes agree", async () => {
  const source = await readFile(
    new URL("../experiments/stack-call.c", import.meta.url),
    "utf8",
  );
  assert.equal(
    createHash("sha256").update(source).digest("hex"),
    trace.provenance.source.sha256,
  );
  assert.equal(source, trace.provenance.source.content);
  assert.match(trace.provenance.binary.sha256, /^[a-f0-9]{64}$/);
  for (const sample of trace.samples) {
    for (const [i, word] of sample.stack.entries()) {
      assert.equal(
        BigInt(word.address),
        BigInt(sample.registers.rsp) + BigInt(i * 8),
      );
      const bytes = word.bytes.split(" ").map((b) => BigInt(`0x${b}`));
      assert.equal(
        bytes.reduce(
          (value, byte, offset) => value + (byte << BigInt(offset * 8)),
          0n,
        ),
        BigInt(word.value),
      );
    }
  }
});
test("GDB capture: missing/forged evidence is rejected, not replaced by model constants", () => {
  const brokenReturn = structuredClone(trace);
  eventOf(brokenReturn, "caller_entry").stack[0].value = "0x1234";
  assert.throws(() => validateStackTrace(brokenReturn), /saved return/);
  const missing = structuredClone(trace);
  delete missing.events.add_ret;
  assert.throws(() => validateStackTrace(missing), /Missing GDB event/);
});
test("GDB renderer does not retain old hand-made stack addresses", async () => {
  const source = await readFile(
    new URL("../chapter3/stack-frame.js", import.meta.url),
    "utf8",
  );
  assert.doesNotMatch(source, /0x[0-9a-f]{4,}/i);
  assert.match(source, /stack-trace\.js/);
});
