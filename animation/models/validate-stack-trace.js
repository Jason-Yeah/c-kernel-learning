// Validation is independent of the renderer and does not invent missing events.
export const eventOf = (trace, name) => {
  const index = trace.events[name];
  if (!Number.isInteger(index) || !trace.samples[index])
    throw new Error(`Missing GDB event: ${name}`);
  return trace.samples[index];
};
export const variableOf = (sample, fn, name) => {
  const value = sample.frames.find((frame) => frame.function === fn)?.variables[
    name
  ];
  if (!value)
    throw new Error(`GDB did not locate ${fn}.${name} at ${sample.pc}`);
  return value;
};
export const wordAt = (sample, address) => {
  const item = sample.stack.find(
    (word) => BigInt(word.address) === BigInt(address),
  );
  if (!item)
    throw new Error(`Address ${address} was not sampled at ${sample.pc}`);
  return item;
};
export function validateStackTrace(trace) {
  const expect = (ok, message) => {
    if (!ok) throw new Error(`Invalid measured stack trace: ${message}`);
  };
  const e = (name) => eventOf(trace, name);
  const same = (a, b) => BigInt(a) === BigInt(b);
  const [
    main,
    callerEntry,
    callerBody,
    beforeAdd,
    addEntry,
    addBody,
    result,
    addRet,
    callerAfter,
    callerRet,
    mainAfter,
    stored,
  ] = [
    "main_before_call",
    "caller_entry",
    "caller_frame_ready",
    "caller_before_add",
    "add_entry",
    "add_frame_ready",
    "add_result_ready",
    "add_ret",
    "caller_after_add",
    "caller_ret",
    "main_after_call",
    "main_result_stored",
  ].map(e);
  expect(
    trace.architecture === "i386:x86-64",
    "collector only supports the documented x86-64 experiment",
  );
  expect(trace.exitCode === 0, "experiment did not exit normally");
  expect(
    trace.samples.every((s, i) => s.index === i && same(s.pc, s.registers.rip)),
    "sample indices or PC inconsistent",
  );
  expect(
    Object.values(trace.events).every((v, i, all) => !i || v > all[i - 1]),
    "named events are not ordered",
  );
  expect(same(main.registers.rdi, 3), "main parameter is not 3");
  expect(
    same(beforeAdd.registers.rdi, 3) && same(beforeAdd.registers.rsi, 2),
    "add register arguments differ",
  );
  for (const [before, entry, after] of [
    [main, callerEntry, mainAfter],
    [beforeAdd, addEntry, callerAfter],
  ]) {
    expect(
      BigInt(before.registers.rsp) - BigInt(entry.registers.rsp) === 8n,
      "call did not push 8 bytes",
    );
    expect(
      same(entry.stack[0].value, after.pc),
      "saved return does not equal the actual instruction after call",
    );
    expect(
      same(after.registers.rsp, before.registers.rsp),
      "ret did not restore the caller stack pointer",
    );
  }
  for (const [entry, body] of [
    [callerEntry, callerBody],
    [addEntry, addBody],
  ]) {
    const rbp = BigInt(body.registers.rbp);
    expect(
      same(wordAt(body, rbp).value, entry.registers.rbp),
      "saved rbp does not match entry",
    );
    expect(
      same(wordAt(body, rbp + 8n).value, entry.stack[0].value),
      "frame return slot is inconsistent",
    );
    expect(
      rbp > BigInt(body.registers.rsp),
      "expected explicit allocated local space with documented flags",
    );
  }
  expect(
    BigInt(main.registers.rsp) % 16n === 0n &&
      BigInt(beforeAdd.registers.rsp) % 16n === 0n,
    "documented GCC no-IPA-alignment experiment changed",
  );
  expect(
    variableOf(callerBody, "caller", "local").value === 3,
    "caller.local is not initialized to 3",
  );
  expect(
    variableOf(addBody, "add", "a").value === 3 &&
      variableOf(addBody, "add", "b").value === 2,
    "homed add parameters differ",
  );
  expect(
    variableOf(result, "add", "sum").value === 5 &&
      same(result.registers.rax, 5),
    "result-ready sample is not 5",
  );
  expect(
    same(addRet.registers.rsp, addEntry.registers.rsp) &&
      same(addRet.registers.rbp, callerBody.registers.rbp),
    "add epilogue restore mismatch",
  );
  expect(
    same(callerRet.registers.rsp, callerEntry.registers.rsp) &&
      same(callerRet.registers.rbp, main.registers.rbp),
    "caller epilogue restore mismatch",
  );
  expect(
    same(mainAfter.registers.rsp, main.registers.rsp) &&
      same(mainAfter.registers.rbp, main.registers.rbp),
    "main context not restored",
  );
  expect(
    variableOf(stored, "main", "result").value === 5,
    "main result store not observed",
  );
  for (const name of ["add_ret", "caller_ret"]) {
    const at = e(name),
      after = trace.samples[at.index + 1];
    expect(
      at.instruction.startsWith("ret"),
      "ret event is not a ret instruction",
    );
    expect(
      same(after.pc, at.stack[0].value) &&
        BigInt(after.registers.rsp) === BigInt(at.registers.rsp) + 8n,
      "instruction-step ret transition not observed",
    );
  }
  return {
    samples: trace.samples.length,
    events: Object.keys(trace.events).length,
  };
}
