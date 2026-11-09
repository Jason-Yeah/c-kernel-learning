import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { createHash } from "node:crypto";
import { resolve } from "node:path";
import { parseELF, elfSymbol, bytesAtVA } from "../models/elf.js";
import {
  validateInstructionCycle,
  signed32,
} from "../models/validate-instruction-cycle.js";
const root = resolve(import.meta.dirname, ".."),
  t = JSON.parse(
    await readFile(resolve(root, "traces/instruction-cycle.json"), "utf8"),
  );
const binary = await readFile(resolve(root, t.provenance.binary.path)),
  elf = parseELF(binary),
  sha = (b) => createHash("sha256").update(b).digest("hex");
const event = (name) => t.samples[t.events[name]];
test("instruction cycle: actual complete source, ELF hashes and code bytes", async () => {
  const result = validateInstructionCycle(t);
  assert.equal(result.samples, t.samples.length);
  assert.equal(result.events, 9);
  assert.equal(sha(binary), t.provenance.binary.sha256);
  const source = await readFile(
    resolve(root, t.provenance.source.path),
    "utf8",
  );
  assert.equal(source, t.provenance.source.content);
  assert.equal(sha(source), t.provenance.source.sha256);
  const base =
    BigInt(t.function.address) - BigInt(elfSymbol(elf, "calculate").value);
  assert.equal(base, BigInt(t.loadBias));
  for (const s of t.samples)
    assert.deepEqual(
      bytesAtVA(
        binary,
        elf,
        BigInt(s.registers.rip) - base,
        s.instruction.length,
      ),
      s.instruction.bytes,
    );
  assert.equal(
    BigInt(event("load").inputs.address),
    base + BigInt(elfSymbol(elf, "inputs").value),
  );
  assert.equal(
    BigInt(event("store").output.address),
    base + BigInt(elfSymbol(elf, "observed").value),
  );
});
test("instruction cycle: independent original GDB register, code and data output", async () => {
  const log = await readFile(
    resolve(root, "traces/instruction-cycle-gdb.txt"),
    "utf8",
  );
  const bx = (text) =>
    text.split("\n").flatMap((line) => {
      const p = line.indexOf(":");
      return p < 0
        ? []
        : [...line.slice(p + 1).matchAll(/0x([\da-f]{2})(?![\da-f])/gi)].map(
            (m) => parseInt(m[1], 16),
          );
    });
  for (const s of t.samples) {
    for (const c of s.commands)
      assert.ok(log.includes(`(gdb) ${c.command}\n${c.output}`));
    const regs = s.commands.find((c) =>
      c.command.startsWith("info registers"),
    ).output;
    for (const m of regs.matchAll(/^([a-z]+)\s+(0x[\da-f]+|\d+)/gim)) {
      assert.equal(
        BigInt(s.registers[m[1]]),
        BigInt.asUintN(
          ["eax", "edx", "eflags"].includes(m[1]) ? 32 : 64,
          BigInt(m[2]),
        ),
      );
    }
    assert.deepEqual(
      bx(s.commands.find((c) => /bx \$rip$/.test(c.command)).output),
      s.instruction.bytes,
    );
    assert.deepEqual(
      bx(s.commands.find((c) => c.command === "x/8bx inputs").output),
      s.inputs.bytes,
    );
    assert.deepEqual(
      bx(s.commands.find((c) => c.command === "x/4bx &observed").output),
      s.output.bytes,
    );
  }
  assert.ok(log.toLowerCase().includes(t.returnAddress.toLowerCase()));
});
test("instruction cycle: software-verified MOV addressing, ADD, store and RET states", () => {
  for (const name of ["load", "store"]) {
    const s = event(name),
      disp = Buffer.from(s.instruction.bytes).readInt32LE(2);
    const ea =
      BigInt(s.registers.rip) + BigInt(s.instruction.length) + BigInt(disp);
    assert.equal(
      ea,
      BigInt(name === "load" ? s.inputs.address : s.output.address),
    );
  }
  assert.equal(BigInt(event("loaded").registers.eax), 3n);
  assert.equal(
    BigInt(event("add").registers.eax) + BigInt(event("add").registers.edx),
    BigInt(event("added").registers.eax),
  );
  for (const s of t.samples) {
    assert.deepEqual(s.inputs.values, [3, 2]);
    assert.equal(s.output.value, s.index <= t.events.store ? 0 : 5);
  }
  assert.equal(event("returned").registers.rip, t.returnAddress);
  assert.notEqual(t.returnAddress, event("returned").registers.rax);
  assert.equal(t.exitCode, 0);
  assert.equal(signed32([255, 255, 255, 255]), -1n);
  assert.equal(signed32([0, 0, 0, 128]), -2147483648n);
});
test("instruction cycle: corrupted or missing evidence fails closed", () => {
  for (const corrupt of [
    (x) => x.samples.shift(),
    (x) => (x.samples[x.events.load].instruction.bytes[0] = 0x90),
    (x) => (x.samples[x.events.added].registers.eax = "0xBAD"),
    (x) => (x.samples[x.events.store].output.value = 5),
    (x) => (x.samples[x.events.returned].registers.rip = "0x1234"),
    (x) => (x.samples[x.events.loaded].registers.rip = "0x12"),
  ]) {
    const copy = structuredClone(t);
    corrupt(copy);
    assert.throws(
      () => validateInstructionCycle(copy),
      /Instruction trace rejected/,
    );
  }
});
