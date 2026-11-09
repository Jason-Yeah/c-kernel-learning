const requireThat = (condition, message) => {
  if (!condition) throw new Error(`Invalid experimental evidence: ${message}`);
};
const same = (a, b) => JSON.stringify(a) === JSON.stringify(b);
const address = (value) => BigInt(value);
export const phaseOf = (run, phase) =>
  run.samples.find(
    (sample) =>
      sample.phase === phase &&
      sample.event !== "signal" &&
      sample.event !== "ret",
  );
export const byteWord = (bytes, offset) =>
  bytes
    .slice(offset, offset + 8)
    .reduce((word, byte, i) => word | (BigInt(byte) << BigInt(i * 8)), 0n);

export function validateExperimentTrace(trace) {
  requireThat(
    trace.schemaVersion === 1 && trace.provenance && trace.runs,
    "schema/provenance/runs missing",
  );
  requireThat(
    /^[a-f0-9]{64}$/.test(trace.provenance.source.sha256) &&
      /^[a-f0-9]{64}$/.test(trace.provenance.binary.sha256),
    "source and ELF fingerprints required",
  );
  for (const run of Object.values(trace.runs)) {
    requireThat(
      run.samples.length > 0 && run.architecture.includes("x86-64"),
      "real x86-64 stops required",
    );
    for (const sample of run.samples) {
      requireThat(
        sample.pid > 0 && sample.commands.length > 0,
        "PID and executed GDB commands required",
      );
      requireThat(
        Object.values(sample.registers).every((value) =>
          /^0x[0-9A-F]+$/i.test(value),
        ),
        "exact register addresses required",
      );
      requireThat(
        sample.commands.every(
          (item) => item.command && typeof item.output === "string",
        ),
        "raw GDB output missing",
      );
    }
  }
  if (trace.id === "buffer-copy") {
    const expected = {
      safe: [..."CSAPP!!"].map((c) => c.charCodeAt(0)).concat(0),
      edge: [..."CSAPP!!!"].map((c) => c.charCodeAt(0)).concat(0),
      broken: [...Array(24).fill(0x41), 0xad, 0xde, 0],
    };
    for (const key of Object.keys(expected)) {
      const run = trace.runs[key],
        ready = phaseOf(run, 1),
        done = phaseOf(run, 3);
      requireThat(ready && done, `${key}: copy boundaries missing`);
      const base = ready.fields,
        retOffset = Number(
          address(base.returnSlot) - address(base.bufferAddress),
        );
      const rbpOffset = Number(
        address(base.rbpSlot) - address(base.bufferAddress),
      );
      requireThat(
        base.capacity === 8 && rbpOffset === 16 && retOffset === 24,
        `${key}: controlled compiler layout differs; adapt explicitly, do not guess`,
      );
      requireThat(
        same(base.input, expected[key]) &&
          same(base.memory.slice(0, 8), Array(8).fill(0xcc)),
        `${key}: source or initialized buf mismatch`,
      );
      const writes = run.samples.filter(
        (s) => s.phase === 2 && s.event !== "signal",
      );
      requireThat(
        writes.length === base.input.length,
        `${key}: missing byte writes`,
      );
      let previous = base.memory;
      writes.forEach((sample, i) => {
        const memory = [...previous];
        memory[i] = base.input[i];
        requireThat(
          sample.fields.index === i && same(sample.fields.input, base.input),
          `${key}/${i}: source mutated or wrong copy index`,
        );
        requireThat(
          same(sample.fields.memory, memory),
          `${key}/${i}: byte changed before/without its write`,
        );
        requireThat(
          byteWord(memory, retOffset) === address(sample.fields.savedReturn),
          `${key}/${i}: real little-endian return slot mismatch`,
        );
        previous = memory;
      });
      const ret = run.samples.find((s) => s.event === "ret");
      requireThat(
        ret && address(ret.registers.rsp) === address(base.returnSlot),
        `${key}: ret did not read captured stack top`,
      );
      if (key === "broken") {
        const fault = run.samples.find((s) => s.event === "signal");
        requireThat(
          run.outcome === "signal" &&
            fault?.signal.name === "SIGSEGV" &&
            fault.signal.code === 1,
          "actual unmapped instruction fetch required",
        );
        requireThat(
          address(fault.registers.rip) === address(done.fields.savedReturn) &&
            address(fault.signal.address) === address(done.fields.savedReturn),
          "fault PC/si_addr must equal the actually overwritten return target",
        );
      } else {
        requireThat(
          run.outcome === "exit" &&
            run.exits.some((e) => e.code === 0) &&
            phaseOf(run, 4),
          `${key}: normal return not observed`,
        );
        requireThat(
          done.fields.savedReturn === base.savedReturn &&
            done.fields.savedRbp === base.savedRbp,
          `${key}: saved control data should remain intact`,
        );
      }
    }
  } else if (trace.id === "stack-recursion") {
    const bounded = trace.runs.bounded,
      exhausted = trace.runs.exhaust;
    requireThat(
      same(
        bounded.samples.filter((s) => s.phase === 1).map((s) => s.fields.depth),
        [1, 2, 3, 4],
      ),
      "bounded recursion entry order",
    );
    requireThat(
      same(
        bounded.samples.filter((s) => s.phase === 2).map((s) => s.fields.depth),
        [4, 3, 2, 1],
      ) && phaseOf(bounded, 3)?.fields.result === 10,
      "actual normal returns and result missing",
    );
    const enters = exhausted.samples.filter(
      (s) => s.phase === 1 && s.event !== "signal",
    );
    requireThat(
      enters.length > 8 &&
        enters.every(
          (s, i) => s.fields.depth === i + 1 && s.fields.stackLimit === 262144,
        ),
      "exhaustion depth/limit evidence",
    );
    const frameBytes =
      address(enters[0].registers.rsp) - address(enters[1].registers.rsp);
    requireThat(
      frameBytes >= 4096n &&
        frameBytes < 8192n &&
        enters
          .slice(1)
          .every(
            (s, i) =>
              address(enters[i].registers.rsp) - address(s.registers.rsp) ===
              frameBytes,
          ),
      "measured recursion frame stride inconsistent",
    );
    const fault = exhausted.samples.find((s) => s.event === "signal");
    requireThat(
      fault?.signal.name === "SIGSEGV" &&
        fault.fields.recursiveFrames.length >= enters.length &&
        exhausted.outcome === "signal",
      "actual stack exhaustion fault/backtrace missing",
    );
  } else if (trace.id === "cache-access") {
    const run = trace.runs.default,
      ready = phaseOf(run, 1),
      loads = run.samples.filter((s) => s.phase === 2);
    requireThat(
      ready && ready.fields.memory.length === 256 && loads.length === 8,
      "array/load observations missing",
    );
    requireThat(
      ready.fields.memory.every((byte, i) => byte === (7 * i + 0x11) % 256),
      "actual C initialization mismatch",
    );
    requireThat(
      same(ready.fields.offsets, [0, 3, 32, 0, 64, 32, 8, 12]),
      "actual C request sequence mismatch",
    );
    loads.forEach((sample, i) => {
      const f = sample.fields,
        value = ready.fields.memory[ready.fields.offsets[i]];
      requireThat(
        f.step === i &&
          f.offset === ready.fields.offsets[i] &&
          address(f.target) - address(ready.fields.base) === BigInt(f.offset),
        "load VA does not match C array offset",
      );
      requireThat(
        f.value === value &&
          f.sink === value &&
          f.byte === value &&
          f.al === value,
        "observed load/byte/register disagree",
      );
    });
  } else if (trace.id === "process-lifecycle") {
    for (const [key, output] of [
      ["early", "PC"],
      ["late", "CP"],
    ]) {
      const run = trace.runs[key],
        parent = phaseOf(run, 1),
        child = phaseOf(run, 2),
        worker = phaseOf(run, 3),
        reaped = phaseOf(run, 6);
      requireThat(
        parent && child && worker && reaped && phaseOf(run, 4),
        `${key}: fork/exec/wait stops missing`,
      );
      requireThat(
        parent.fields.forkResult === child.pid &&
          parent.fields.childPid === child.pid &&
          child.fields.forkResult === 0,
        `${key}: actual fork returns disagree`,
      );
      requireThat(
        worker.pid === child.pid &&
          run.samples.some((s) => s.event === "exec" && s.pid === child.pid),
        `${key}: same-PID exec not actually observed`,
      );
      requireThat(
        reaped.fields.waitResult === child.pid &&
          reaped.fields.waitStatus === 0x700 &&
          run.stdout === output,
        `${key}: wait status or actual stdout mismatch`,
      );
      if (key === "late")
        requireThat(
          phaseOf(run, 5)?.fields.childState?.state === "Z",
          "actual unreaped zombie not observed",
        );
      requireThat(
        same(
          run.exits.map((e) => e.code).sort((a, b) => a - b),
          [0, 7],
        ),
        `${key}: parent/worker completion missing`,
      );
    }
  } else if (trace.id === "fork-memory") {
    const run = trace.runs.default,
      p = phaseOf(run, 1),
      cb = phaseOf(run, 2),
      ca = phaseOf(run, 3),
      pa = phaseOf(run, 4);
    requireThat(
      p &&
        cb &&
        ca &&
        pa &&
        p.pid !== cb.pid &&
        cb.pid === ca.pid &&
        p.pid === pa.pid,
      "parent/child observations missing",
    );
    requireThat(
      [cb, ca, pa].every(
        (s) =>
          s.fields.privateAddress === p.fields.privateAddress &&
          s.fields.sharedAddress === p.fields.sharedAddress,
      ),
      "same inherited VA evidence missing",
    );
    requireThat(
      same(
        [p, cb, ca, pa].map((s) => [
          s.fields.privateValue,
          s.fields.sharedValue,
        ]),
        [
          [40, 50],
          [40, 50],
          [400, 500],
          [40, 500],
        ],
      ),
      "private/shared actual values disagree",
    );
    requireThat(
      same(
        [p, cb, ca, pa].map((s) => [
          s.fields.globalData,
          s.fields.globalBss,
          s.fields.localValue,
          s.fields.heapValue,
        ]),
        [
          [10, 0, 20, 30],
          [10, 0, 20, 30],
          [100, 200, 200, 300],
          [10, 0, 20, 30],
        ],
      ),
      "actual data/bss/local/heap isolation differs",
    );
    requireThat(
      p.fields.privateMapping.includes("rw-p") &&
        p.fields.sharedMapping.includes("rw-s"),
      "actual mapping permissions absent",
    );
    requireThat(
      same(
        run.exits.map((e) => e.code),
        [0, 0],
      ),
      "fork experiment did not finish normally",
    );
  } else if (trace.id === "vm-access") {
    for (const [key, value] of [
      ["resident-a", 0x7b],
      ["resident-b", 0xa6],
      ["cold", 0],
    ]) {
      const run = trace.runs[key],
        before = phaseOf(run, 1),
        after = phaseOf(run, 2);
      requireThat(
        before &&
          after &&
          after.fields.value === value &&
          after.fields.byte === value &&
          after.fields.page.present,
        `${key}: completed read evidence missing`,
      );
      requireThat(
        before.fields.pageSize === 4096 &&
          address(before.fields.target) % 4096n === 0x2dn,
        `${key}: actual page size/offset mismatch`,
      );
      if (key === "cold")
        requireThat(
          !before.fields.page.present &&
            after.fields.afterFaults - after.fields.beforeFaults >= 1,
          "cold access must actually change present from 0 to 1",
        );
      else
        requireThat(
          before.fields.page.present,
          "resident page must already be present",
        );
    }
    for (const [key, code] of [
      ["readonly", 2],
      ["unmapped", 1],
    ]) {
      const run = trace.runs[key],
        before = phaseOf(run, 1),
        fault = run.samples.find((s) => s.event === "signal");
      requireThat(
        before &&
          fault?.signal.name === "SIGSEGV" &&
          fault.signal.code === code &&
          fault.signal.address === before.fields.target &&
          run.outcome === "signal",
        `${key}: actual signal/address mismatch`,
      );
      if (key === "readonly")
        requireThat(
          before.fields.mapping.split(/\s+/)[1] === "r--p" &&
            fault.fields.byte === 0xaa,
          "protected byte must remain unchanged",
        );
      else
        requireThat(
          before.fields.mapping === null,
          "unmapped target must not have a mapping",
        );
    }
  } else {
    throw new Error(`Unsupported experiment ${trace.id}`);
  }
  return {
    runs: Object.keys(trace.runs).length,
    samples: Object.values(trace.runs).reduce(
      (sum, run) => sum + run.samples.length,
      0,
    ),
  };
}
