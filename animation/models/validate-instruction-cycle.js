const fact = (ok, message) => {
  if (!ok) throw new Error("Instruction trace rejected: " + message);
};
export const signed32 = (bytes) => {
  fact(bytes.length === 4, "disp32 bytes");
  let n = bytes.reduce((n, b, i) => n + (BigInt(b) << BigInt(i * 8)), 0n);
  return BigInt.asIntN(32, n);
};
export const effectiveAddress = (s) =>
  BigInt(s.registers.rip) +
  BigInt(s.instruction.length) +
  signed32(s.instruction.bytes.slice(2));
export function identifyCycleEvents(samples) {
  const load = samples.findIndex(
    (s) =>
      s.instruction.bytes[0] === 0x8b &&
      s.instruction.bytes[1] === 0x05 &&
      s.instruction.length === 6 &&
      effectiveAddress(s) === BigInt(s.inputs.address),
  );
  const add = samples.findIndex(
    (s) =>
      s.instruction.bytes.length === 2 &&
      s.instruction.bytes[0] === 1 &&
      s.instruction.bytes[1] === 0xd0,
  );
  const store = samples.findIndex(
    (s) =>
      s.instruction.bytes[0] === 0x89 &&
      s.instruction.bytes[1] === 0x05 &&
      s.instruction.length === 6 &&
      effectiveAddress(s) === BigInt(s.output.address),
  );
  const ret = samples.findIndex((s) => s.instruction.asm.startsWith("ret"));
  fact(
    load >= 0 && add > load && store > add && ret > store,
    "reviewed MOV/ADD/STORE/RET sequence absent",
  );
  return {
    entry: 0,
    load,
    loaded: load + 1,
    add,
    added: add + 1,
    store,
    stored: store + 1,
    ret,
    returned: ret + 1,
  };
}
export function validateInstructionCycle(t) {
  fact(t?.schemaVersion === 1 && t.id === "instruction-cycle", "schema");
  fact(
    t.provenance?.source?.content &&
      /^[a-f\d]{64}$/.test(t.provenance.source.sha256) &&
      /^[a-f\d]{64}$/.test(t.provenance.binary.sha256),
    "complete provenance",
  );
  fact(
    t.exitCode === 0 && t.stdout.includes("observed=5 return=5"),
    "actual success",
  );
  const samples = t.samples;
  fact(samples.length > 8, "instruction stops");
  const events = identifyCycleEvents(samples);
  fact(
    JSON.stringify(events) === JSON.stringify(t.events),
    "derived event indices",
  );
  const event = (name) => samples[events[name]];
  const input = event("entry").inputs.address,
    output = event("entry").output.address;
  fact(event("entry").output.value === 0, "explicit zero-initialized output");
  for (const [i, s] of samples.entries()) {
    fact(
      s.index === i && s.pid === samples[0].pid && s.pid > 0,
      "one real process",
    );
    fact(
      s.instruction.address === s.registers.rip &&
        s.instruction.length === s.instruction.bytes.length,
      "RIP/byte length",
    );
    fact(s.commands?.length >= 7, "raw GDB commands");
    fact(
      s.inputs.address === input &&
        s.output.address === output &&
        s.inputs.values[0] === 3 &&
        s.inputs.values[1] === 2,
      "unchanged source inputs",
    );
    fact(s.inputs.bytes.join(",") === "3,0,0,0,2,0,0,0", "actual input bytes");
    fact(
      s.output.value === (i <= events.store ? 0 : 5),
      "store is the only output change",
    );
    fact(
      s.output.bytes.join(",") === (i <= events.store ? "0,0,0,0" : "5,0,0,0"),
      "little-endian output bytes",
    );
    if (i < events.ret)
      fact(
        BigInt(samples[i + 1].registers.rip) ===
          BigInt(s.registers.rip) + BigInt(s.instruction.length),
        "sequential RIP + actual instruction length",
      );
  }
  fact(BigInt(event("loaded").registers.eax) === 3n, "MOV loaded operand");
  fact(
    BigInt(event("add").registers.eax) === 2n &&
      BigInt(event("add").registers.edx) === 3n,
    "actual ADD operands",
  );
  fact(
    BigInt(event("added").registers.eax) === 5n &&
      BigInt(event("added").registers.rax) === 5n,
    "32-bit ADD result",
  );
  fact(
    event("stored").output.value ===
      Number(BigInt(event("store").registers.eax)),
    "actual store source/destination",
  );
  fact(
    event("returned").registers.rip === t.returnAddress,
    "RET uses saved return address",
  );
  fact(
    BigInt(event("returned").registers.rsp) ===
      BigInt(event("entry").registers.rsp) + 8n,
    "ret stack restoration",
  );
  fact(
    BigInt(event("returned").registers.rax) === 5n,
    "integer return distinct from return address",
  );
  fact(samples.at(-1).source.function === "main", "actual return to main");
  return { samples: samples.length, events: Object.keys(events).length };
}
