import test from "node:test";
import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { createHash } from "node:crypto";
import { resolve } from "node:path";
import { parseELF, elfSymbol } from "../models/elf.js";
import {
  validateOSBasics,
  osSample,
  osObject,
} from "../models/validate-os-basics.js";
const root = resolve(import.meta.dirname, "..");
const t = JSON.parse(
  await readFile(resolve(root, "traces/os-basics.json"), "utf8"),
);
const sha = (bytes) => createHash("sha256").update(bytes).digest("hex");
const bytes = await readFile(resolve(root, "traces/os-basics/os-demo.elf"));
const mainBytes = await readFile(resolve(root, "traces/os-basics/os-main.o"));
const mathBytes = await readFile(resolve(root, "traces/os-basics/os-math.o"));
const exe = parseELF(bytes),
  main = parseELF(mainBytes),
  math = parseELF(mathBytes);
test("OS: all three real runs, full C/header and every artifact hash", async () => {
  const summary = validateOSBasics(t);
  assert.equal(summary.runs, 3);
  assert.equal(
    summary.samples,
    Object.values(t.runs).reduce((n, r) => n + r.samples.length, 0),
  );
  assert.ok(summary.samples > 20);
  for (const source of t.provenance.sources) {
    assert.equal(
      await readFile(resolve(root, source.path), "utf8"),
      source.content,
    );
    assert.equal(sha(source.content), source.sha256);
  }
  for (const file of t.build.artifacts) {
    const raw = await readFile(resolve(root, file.path));
    assert.equal(raw.length, file.size);
    assert.equal(sha(raw), file.sha256);
  }
  assert.equal(sha(bytes), t.provenance.binary.sha256);
  assert.deepEqual(exe, t.elf.executable);
  assert.deepEqual(main, t.elf.main);
  assert.deepEqual(math, t.elf.math);
});
test("OS: actual ELF headers, symbol sections, NOBITS and rejected corrupt files", () => {
  assert.equal(
    new DataView(mainBytes.buffer, mainBytes.byteOffset).getUint16(16, true),
    1,
  );
  assert.equal(
    new DataView(bytes.buffer, bytes.byteOffset).getUint16(16, true),
    3,
  );
  assert.match(
    t.reports.find((r) => r.key === "object-header").output,
    /REL \(Relocatable file\)/,
  );
  assert.match(
    t.reports.find((r) => r.key === "executable-header").output,
    /DYN \(Position-Independent Executable file\)/,
  );
  assert.equal(elfSymbol(math, "os_bss").section, ".bss");
  assert.equal(elfSymbol(math, "os_data").section, ".data");
  assert.equal(math.sections.find((s) => s.name === ".bss").fileBytes, 0);
  const d = elfSymbol(math, "os_data"),
    section = math.sections.find((s) => s.name === ".data");
  assert.equal(
    new DataView(mathBytes.buffer, mathBytes.byteOffset).getInt32(
      section.offset + Number(BigInt(d.value)),
      true,
    ),
    10,
  );
  const corrupt = Uint8Array.from(mainBytes);
  corrupt[0] = 0;
  assert.throws(() => parseELF(corrupt), /ELF evidence/);
  assert.throws(() => parseELF(mainBytes.slice(0, 80)), /ELF evidence/);
});
test("OS: PC32 patch independently matches raw executable bytes and objdump", () => {
  const r = t.relocation,
    P = BigInt(r.P),
    S = BigInt(r.S),
    A = BigInt(r.A);
  const segment = exe.programs.find(
    (p) =>
      p.type === 1 &&
      P >= BigInt(p.vaddr) &&
      P + 4n <= BigInt(p.vaddr) + BigInt(p.fileSize),
  );
  const offset = segment.offset + Number(P - BigInt(segment.vaddr));
  const raw = new DataView(bytes.buffer, bytes.byteOffset).getInt32(
    offset,
    true,
  );
  assert.equal(BigInt(raw), S + A - P);
  assert.equal(P + 4n + BigInt(raw), S);
  assert.deepEqual([...bytes.slice(offset, offset + 4)], r.after);
  const objectText = main.sections.find((s) => s.name === ".text");
  const objectOff = objectText.offset + Number(BigInt(r.objectOffset));
  assert.deepEqual([...mainBytes.slice(objectOff, objectOff + 4)], r.before);
  const target = S.toString(16);
  assert.ok(
    t.reports
      .find((r) => r.key === "executable-disassembly")
      .output.includes(`${target} <os_data>`),
  );
});
test("OS: every register is independently checked against original GDB text", async () => {
  const log = await readFile(resolve(root, "traces/os-basics-gdb.txt"), "utf8");
  for (const run of Object.values(t.runs))
    for (const sample of run.samples) {
      const registers = sample.commands.find((c) =>
        c.command.startsWith("info registers"),
      ).output;
      for (const match of registers.matchAll(
        /^([a-z0-9]+)\s+(0x[\da-f]+|\d+)/gim,
      )) {
        assert.equal(
          BigInt(sample.registers[match[1]]),
          BigInt.asUintN(64, BigInt(match[2])),
        );
      }
      for (const command of sample.commands)
        assert.ok(log.includes(`(gdb) ${command.command}\n${command.output}`));
    }
  assert.equal(
    osSample(t, "main-entry").objects,
    undefined,
    "do not display uninitialized C variables at entry",
  );
});
test("OS: actual PT_INTERP, argc/argv, symbol load bias and inherited VAs", () => {
  for (const run of ["first", "second"]) {
    const interp = osSample(t, "interpreter-entry", run),
      entry = osSample(t, "app-entry", run),
      main = osSample(t, "main-entry", run),
      ready = osSample(t, "layout-ready", run);
    const pc = BigInt(interp.registers.rip);
    assert.ok(
      interp.maps.some((line) => {
        const [a, b] = line
          .split(" ")[0]
          .split("-")
          .map((s) => BigInt("0x" + s));
        return pc >= a && pc < b && line.includes("ld-linux");
      }),
    );
    assert.equal(BigInt(entry.initialStack[0]), 2n);
    assert.equal(BigInt(main.registers.rdi), 2n);
    const bias = BigInt(entry.registers.rip) - BigInt(exe.header.entry);
    for (const [objectName, symbolName] of [
      ["text", "main"],
      ["rodata", "os_message"],
      ["data", "os_data"],
      ["bss", "os_bss"],
    ])
      assert.equal(
        BigInt(osObject(ready, objectName).address),
        bias + BigInt(elfSymbol(exe, symbolName).value),
      );
  }
  const p = osSample(t, "phase-2", "fork"),
    c = osSample(t, "phase-3", "fork"),
    after = osSample(t, "phase-5", "fork");
  assert.equal(c.forkResult, 0);
  assert.equal(after.forkResult, c.pid);
  for (const name of ["stack", "heap", "data", "bss", "private", "shared"])
    assert.equal(osObject(p, name).address, osObject(c, name).address);
});
test("OS: missing and forged evidence fail closed", () => {
  for (const corrupt of [
    (x) => (x.relocation.after[0] ^= 1),
    (x) =>
      x.runs.first.samples.splice(
        x.runs.first.samples.findIndex((s) => s.event === "app-entry"),
        1,
      ),
    (x) =>
      (x.runs.first.samples.find(
        (s) => s.event === "os_add-returned",
      ).registers.rax = "0xBAD"),
    (x) =>
      (x.runs.fork.samples
        .find((s) => s.event === "phase-5")
        .objects.find((o) => o.name === "private").value = 400),
    (x) =>
      (x.runs.fork.samples.find((s) => s.event === "phase-3").pid =
        x.runs.fork.samples[0].pid),
  ]) {
    const copy = structuredClone(t);
    corrupt(copy);
    assert.throws(() => validateOSBasics(copy), /OS evidence rejected/);
  }
});
