import cacheExperiment from "../traces/cache-access.json" with { type: "json" };
const cacheObservation = cacheExperiment.runs.default.samples.find(
  (sample) => sample.phase === 1,
).fields;
export const hex = (n, width = 2) =>
  n.toString(16).toUpperCase().padStart(width, "0");
export function splitAddress(address) {
  if (!Number.isInteger(address) || address < 0 || address > 255)
    throw new RangeError("8-bit integer address required");
  return { tag: address >> 5, index: (address >> 3) & 3, offset: address & 7 };
}
export const memoryByte = (address) => cacheObservation.memory[address];
export const requests = cacheObservation.offsets;
export function simulateCache() {
  let cache = Array.from({ length: 4 }, () =>
    Array.from({ length: 2 }, () => ({
      valid: false,
      tag: null,
      block: null,
      data: [],
      used: -1,
    })),
  );
  let hits = 0,
    misses = 0,
    al = null;
  return requests.map((address, number) => {
    const { tag, index, offset } = splitAddress(address);
    const before = structuredClone(cache),
      ways = cache[index];
    const hitWay = ways.findIndex((line) => line.valid && line.tag === tag);
    const empty = ways.findIndex((line) => !line.valid);
    const hit = hitWay >= 0;
    const way = hit
      ? hitWay
      : empty >= 0
        ? empty
        : ways[0].used < ways[1].used
          ? 0
          : 1;
    const victim = structuredClone(ways[way]);
    const block = address & ~7,
      value = memoryByte(address);
    const previous = { al, hits, misses };
    cache[index][way] = {
      valid: true,
      tag,
      block,
      data: Array.from({ length: 8 }, (_, i) => memoryByte(block + i)),
      used: number,
    };
    hit ? hits++ : misses++;
    al = value;
    return {
      number,
      address,
      tag,
      index,
      offset,
      hit,
      way,
      victim,
      empty: empty >= 0,
      block,
      value,
      before,
      after: structuredClone(cache),
      previous,
      hits,
      misses,
    };
  });
}
export const cacheRecords = simulateCache();
export const vmCases = [
  {
    key: "hit",
    vpn: 4,
    pfn: 9,
    cached: true,
    present: true,
    mapped: true,
    writable: true,
    data: 0x7b,
  },
  {
    key: "miss",
    vpn: 5,
    pfn: 2,
    cached: false,
    present: true,
    mapped: true,
    writable: true,
    data: 0xa6,
  },
  {
    key: "page",
    vpn: 6,
    pfn: 11,
    cached: false,
    present: false,
    mapped: true,
    writable: true,
    data: 0,
  },
  {
    key: "denied",
    vpn: 8,
    pfn: 3,
    cached: true,
    present: true,
    mapped: true,
    writable: false,
    data: null,
  },
  {
    key: "unmapped",
    vpn: 7,
    pfn: null,
    cached: false,
    present: false,
    mapped: false,
    writable: false,
    data: null,
  },
].map((s) => ({
  ...s,
  va: s.vpn * 64 + 0x2d,
  pa: s.pfn === null ? null : s.pfn * 64 + 0x2d,
}));
export const lifecycleCases = [
  [
    "start",
    "fork",
    "fork-parent",
    "print-parent",
    "wait-enter",
    "block",
    "exec-enter",
    "exec-done",
    "print-child",
    "exit-enter",
    "exit-wake",
    "reap",
    "continue",
  ],
  [
    "start",
    "fork",
    "fork-child",
    "exec-enter",
    "exec-done",
    "print-child",
    "exit-enter",
    "exit-late",
    "print-parent",
    "other-work",
    "wait-enter",
    "reap",
    "continue",
  ],
];
export function processTrace(events, ids = { parent: 101, child: 202 }) {
  const { parent: parentPid, child: childPid } = ids;
  const s = {
    parent: { pid: parentPid, state: "running", image: "launcher", fork: null },
    child: null,
    cpu: parentPid,
    mode: "user",
    output: "",
    reaped: false,
    wait: null,
    status: null,
  };
  return events.map((event) => {
    if (event === "fork") s.mode = "kernel";
    if (event === "fork-parent" || event === "fork-child") {
      const childFirst = event === "fork-child";
      s.parent.fork = childPid;
      s.parent.state = childFirst ? "ready" : "running";
      s.child = {
        pid: childPid,
        state: childFirst ? "running" : "ready",
        image: "launcher",
        fork: 0,
      };
      s.cpu = childFirst ? childPid : parentPid;
      s.mode = "user";
    }
    if (event === "print-parent") s.output += "P";
    if (
      event === "wait-enter" ||
      event === "exec-enter" ||
      event === "exit-enter"
    )
      s.mode = "kernel";
    if (event === "block") {
      s.parent.state = "waiting";
      s.child.state = "running";
      s.cpu = childPid;
      s.mode = "user";
    }
    if (event === "exec-done") {
      s.child.image = "worker";
      s.child.fork = null;
      s.mode = "user";
    }
    if (event === "print-child") s.output += "C";
    if (event === "exit-wake" || event === "exit-late") {
      s.child.state = "zombie";
      s.child.image = null;
      s.parent.state = "running";
      s.cpu = parentPid;
      s.mode = event === "exit-wake" ? "kernel" : "user";
    }
    if (event === "reap") {
      s.child = null;
      s.reaped = true;
      s.wait = childPid;
      s.status = ids.status ?? 7 << 8;
      s.mode = "user";
    }
    return { ...structuredClone(s), event };
  });
}
export function cowTrace() {
  return [
    {
      phase: "shared",
      parent: "A",
      child: "A",
      A: 40,
      B: null,
      pte: "RO/COW",
      mode: "user",
    },
    {
      phase: "fault",
      parent: "A",
      child: "A",
      A: 40,
      B: null,
      pte: "RO/COW",
      mode: "kernel",
    },
    {
      phase: "copying",
      parent: "A",
      child: "A",
      A: 40,
      B: null,
      pte: "RO/COW",
      mode: "kernel",
    },
    {
      phase: "remap",
      parent: "A",
      child: "B",
      A: 40,
      B: 40,
      pte: "RW",
      mode: "kernel",
    },
    {
      phase: "retried",
      parent: "A",
      child: "B",
      A: 40,
      B: 400,
      pte: "RW",
      mode: "user",
    },
  ];
}
