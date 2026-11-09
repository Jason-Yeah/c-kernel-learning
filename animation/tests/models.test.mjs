import test from "node:test";
import assert from "node:assert/strict";
import {
  splitAddress,
  memoryByte,
  simulateCache,
  cacheRecords,
  vmCases,
  lifecycleCases,
  processTrace,
  cowTrace,
} from "../models/computer.js";
import { films } from "../shared/catalog.js";

test("cache: 256 addresses round-trip, exact eight-access LRU history", () => {
  for (let address = 0; address < 256; address++) {
    const s = splitAddress(address);
    assert.equal((s.tag << 5) + (s.index << 3) + s.offset, address);
  }
  for (const bad of [-1, 256, 1.5, null, true])
    assert.throws(() => splitAddress(bad), RangeError);
  assert.deepEqual(
    cacheRecords.map((r) => r.hit),
    [false, true, false, true, false, false, false, true],
  );
  assert.deepEqual(
    cacheRecords.map((r) => r.way),
    [0, 0, 1, 0, 1, 0, 0, 0],
  );
  assert.equal(cacheRecords[4].victim.block, 0x20);
  assert.equal(cacheRecords[5].victim.block, 0);
  for (const r of cacheRecords) {
    assert.equal(r.after[r.index][r.way].data[r.offset], memoryByte(r.address));
    for (let i = 0; i < 4; i++)
      for (let w = 0; w < 2; w++)
        if (i !== r.index || w !== r.way)
          assert.deepEqual(r.before[i][w], r.after[i][w]);
  }
  assert.equal(cacheRecords.at(-1).value, 0x65);
  assert.equal(cacheRecords.at(-1).hits, 3);
  assert.equal(cacheRecords.at(-1).misses, 5);
  assert.deepEqual(simulateCache(), cacheRecords);
});
test("VM: offset preserved, five distinct paths and permission restrictions", () => {
  assert.deepEqual(
    vmCases.slice(0, 3).map((c) => c.pa),
    [0x26d, 0xad, 0x2ed],
  );
  for (const c of vmCases) {
    assert.equal(c.va & 63, 0x2d);
    if (c.pa !== null) assert.equal(c.pa & 63, 0x2d);
  }
  assert.equal(vmCases[2].mapped, true);
  assert.equal(vmCases[2].present, false);
  assert.equal(vmCases[3].writable, false);
  assert.equal(vmCases[4].mapped, false);
});
test("process: one running owner, no zombie image/CPU, exact wait status", () => {
  for (const events of lifecycleCases) {
    const trace = processTrace(events);
    for (const s of trace) {
      const running = [s.parent, s.child].filter((p) => p?.state === "running");
      assert.equal(running.length, 1);
      assert.equal(running[0].pid, s.cpu);
      if (s.child?.state === "zombie") {
        assert.equal(s.child.image, null);
        assert.notEqual(s.cpu, 202);
      }
      if (s.reaped) {
        assert.equal(s.child, null);
        assert.equal(s.wait, 202);
        assert.equal(s.status, 0x0700);
      }
    }
    assert.equal(trace.at(-1).output, events.includes("block") ? "PC" : "CP");
  }
});
test("COW: B is not mapped before copying, write only after remap/retry", () => {
  const s = cowTrace();
  assert.equal(s[2].B, null);
  assert.equal(s[2].child, "A");
  assert.equal(s[3].B, 40);
  assert.equal(s[3].child, "B");
  assert.equal(s[4].B, 400);
  for (const item of s) {
    assert.equal(item.A, 40);
    assert.equal(item.parent, "A");
  }
});
test("catalog: existing films, OS, instruction cycle, byte order and two integer films, unique stable entries", () => {
  assert.equal(films.length, 18);
  assert.equal(new Set(films.map((f) => f.id)).size, 18);
  assert.equal(films.filter((f) => f.chapter === 0).length, 6);
  assert.equal(
    films.find((f) => f.id === "stack-exhaustion").duration - 12,
    30,
    "the original mechanism segment remains 30s after adding the C/GDB introduction",
  );
  assert.deepEqual(
    [...new Set(films.map((f) => f.chapter))],
    [0, 1, 2, 3, 6, 8, 9],
  );
});
