const requireFact = (ok, message) => {
  if (!ok) throw new Error(`OS evidence rejected: ${message}`);
};
const eq = (a, b) => BigInt(a) === BigInt(b);
export const osSample = (trace, event, run = "first") => {
  const sample = trace.runs?.[run]?.samples.find((s) => s.event === event);
  requireFact(sample, `missing ${run}/${event}`);
  return sample;
};
export const osObject = (sample, name) => {
  const value = sample.objects?.find((o) => o.name === name);
  requireFact(value, `missing object ${name}`);
  return value;
};
export function validateOSBasics(t) {
  requireFact(t?.id === "os-basics" && t.schemaVersion === 1, "schema");
  requireFact(
    t.provenance.sources.length === 3 &&
      t.provenance.sources.every(
        (s) => s.content && /^[a-f0-9]{64}$/.test(s.sha256),
      ),
    "complete sources and hashes",
  );
  requireFact(
    t.build.commands.length >= 7 &&
      t.reports.length >= 8 &&
      t.reports.every((r) => r.command && r.output),
    "actual tool output",
  );
  const elf = t.elf;
  requireFact(
    elf.main.header.type === 1 &&
      elf.math.header.type === 1 &&
      elf.executable.header.type === 3,
    "ET_REL -> PIE ET_DYN",
  );
  requireFact(
    elf.executable.programs.some((p) => p.type === 3 && p.interpreter),
    "PT_INTERP",
  );
  requireFact(
    elf.math.sections.find((s) => s.name === ".bss")?.type === 8,
    "NOBITS BSS",
  );
  const r = t.relocation;
  requireFact(
    r.type === "R_X86_64_PC32" && r.symbol === "os_data",
    "actual PC32 relocation",
  );
  const value = BigInt(r.S) + BigInt(r.A) - BigInt(r.P);
  requireFact(
    value >= -2147483648n && value <= 2147483647n,
    "signed32 displacement",
  );
  const packed = BigInt.asUintN(32, value);
  requireFact(
    r.after.length === 4 &&
      r.after.every((b, i) => b === Number((packed >> BigInt(i * 8)) & 255n)),
    "real linker patch S+A-P",
  );
  requireFact(BigInt(r.P) + 4n + value === BigInt(r.S), "PC-relative target");
  for (const key of ["first", "second", "fork"]) {
    const run = t.runs[key];
    requireFact(
      run?.samples.length >= 5 &&
        run.exits.length > 0 &&
        run.exits.every((e) => e.code === 0),
      "actual normal exits " + key,
    );
    requireFact(
      run.samples.every(
        (s, i) =>
          s.index === i &&
          s.pid > 0 &&
          Object.keys(s.registers).length === 18 &&
          s.commands.length >= 2,
      ),
      "complete GDB snapshots " + key,
    );
  }
  for (const key of ["first", "second"]) {
    const first = osSample(t, "interpreter-entry", key),
      entry = osSample(t, "app-entry", key),
      main = osSample(t, "main-entry", key),
      ready = osSample(t, "layout-ready", key),
      done = osSample(t, "compute-done", key);
    const base =
      BigInt(entry.registers.rip) - BigInt(elf.executable.header.entry);
    requireFact(
      eq(entry.registers.rip, first.auxv["9"]),
      "AT_ENTRY application entry",
    );
    requireFact(
      !eq(first.registers.rip, entry.registers.rip),
      "interpreter before app _start",
    );
    const mainSymbol = elf.executable.symbols.find(
      (s) => s.table === ".symtab" && s.name === "main",
    );
    requireFact(
      BigInt(main.registers.rip) === base + BigInt(mainSymbol.value),
      "PIE load bias -> main",
    );
    requireFact(
      eq(entry.initialStack[0], main.registers.rdi) &&
        BigInt(main.registers.rsi) === BigInt(entry.registers.rsp) + 8n,
      "initial argc/argv -> main ABI",
    );
    requireFact(
      done.result === 5 && eq(done.widthResult, 5) && done.equalResult === 1,
      "actual C results",
    );
    requireFact(
      ready.objects.length === 8 && ready.pageSize === 4096,
      "measured layout",
    );
    for (const [name, value] of Object.entries({
      data: 10,
      bss: 0,
      heap: 30,
      private: 40,
      shared: 50,
      stack: 20,
    })) {
      const object = osObject(ready, name);
      requireFact(object.value === value, "initialized " + name);
      const bits = object.bytes.reduce(
        (n, b, i) => n + (BigInt(b) << BigInt(i * 8)),
        0n,
      );
      requireFact(bits === BigInt(value), "actual bytes " + name);
      requireFact(
        ready.maps.some((line) => {
          const [a, b] = line
            .split(" ")[0]
            .split("-")
            .map((s) => BigInt("0x" + s));
          return BigInt(object.address) >= a && BigInt(object.address) < b;
        }),
        "object lies in observed maps " + name,
      );
    }
  }
  const add = osSample(t, "os_add-entry"),
    ret = osSample(t, "os_add-returned");
  requireFact(
    eq(add.registers.rdi, 3) && eq(add.registers.rsi, 2),
    "argument registers before variable initialization",
  );
  requireFact(
    eq(ret.registers.rax, 5) &&
      BigInt(ret.registers.rsp) === BigInt(add.registers.rsp) + 8n &&
      eq(ret.registers.rip, add.stackTop),
    "actual ret and stack restoration",
  );
  const widthBefore = t.runs.first.samples.find(
    (s) =>
      s.event.startsWith("os_width") && /\$0x5,%eax/.test(s.instruction.asm),
  );
  const widthAfter = t.runs.first.samples.find(
    (s) =>
      s.event.startsWith("os_width") &&
      /\$0x5,%eax/.test(s.previousInstruction?.asm || ""),
  );
  requireFact(
    widthBefore &&
      widthAfter &&
      eq(widthBefore.registers.rax, "0xFFFFFFFFFFFFFFFF") &&
      eq(widthAfter.registers.rax, 5),
    "measured EAX zero extension",
  );
  const flags = t.runs.first.samples.find(
    (s) =>
      s.event.startsWith("os_equal") &&
      s.previousInstruction?.asm.startsWith("cmp"),
  );
  requireFact(
    flags && (BigInt(flags.registers.eflags) & 64n) !== 0n,
    "actual ZF after cmp equal",
  );
  const p = osSample(t, "phase-2", "fork"),
    c = osSample(t, "phase-3", "fork"),
    changed = osSample(t, "phase-4", "fork"),
    after = osSample(t, "phase-5", "fork");
  requireFact(
    p.pid === after.pid &&
      c.pid === changed.pid &&
      c.pid !== p.pid &&
      c.forkResult === 0 &&
      after.forkResult === c.pid &&
      after.childPid === c.pid,
    "actual fork PID and returns",
  );
  for (const name of ["data", "bss", "stack", "heap", "private", "shared"])
    requireFact(
      osObject(p, name).address === osObject(c, name).address,
      "inherited VA " + name,
    );
  for (const [name, value] of Object.entries({
    data: 100,
    bss: 200,
    stack: 200,
    heap: 300,
    private: 400,
    shared: 500,
  }))
    requireFact(osObject(changed, name).value === value, "child write " + name);
  for (const [name, value] of Object.entries({
    data: 10,
    bss: 0,
    stack: 20,
    heap: 30,
    private: 40,
    shared: 500,
  }))
    requireFact(
      osObject(after, name).value === value,
      "parent isolation/shared " + name,
    );
  return {
    runs: 3,
    samples: Object.values(t.runs).reduce((n, r) => n + r.samples.length, 0),
  };
}
