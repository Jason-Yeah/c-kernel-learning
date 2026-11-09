import trace from "../traces/instruction-cycle.json" with { type: "json" };
import {
  validateInstructionCycle,
  effectiveAddress,
  signed32,
} from "./validate-instruction-cycle.js";
export const cycleTrace = trace;
export const cycleSummary = validateInstructionCycle(trace);
export const at = (name) => trace.samples[trace.events[name]];
export const load = at("load"),
  loaded = at("loaded"),
  add = at("add"),
  added = at("added"),
  store = at("store"),
  stored = at("stored"),
  ret = at("ret"),
  returned = at("returned");
export const hx = (n) => "0x" + BigInt(n).toString(16).toUpperCase();
export const byte = (n) => n.toString(16).toUpperCase().padStart(2, "0");
export const byteString = (list) => list.map(byte).join(" ");
export const loadEA = hx(effectiveAddress(load));
export const loadDisp = signed32(load.instruction.bytes.slice(2));
export const storeEA = hx(effectiveAddress(store));
export function sourcePart(needles) {
  const lines = trace.provenance.source.content.split("\n");
  const numbers = needles.map((term) => {
    const indexes = lines.flatMap((s, i) => (s.includes(term) ? [i + 1] : []));
    if (indexes.length !== 1)
      throw new Error("Ambiguous measured source " + term);
    return indexes[0];
  });
  return { lines: numbers.map((n) => lines[n - 1].trim()), numbers };
}
export function inputRows(sample = load) {
  return [1, 0].map((i) => ({
    key: String(i),
    label: `inputs[${i}] / 原始输入`,
    address: hx(BigInt(sample.inputs.address) + BigInt(4 * i)),
    value: sample.inputs.values[i],
    role: "source-row",
  }));
}
export const codeRows = load.instruction.bytes
  .map((v, i) => ({
    key: String(i),
    label: `指令 byte[${i}]`,
    address: hx(BigInt(load.registers.rip) + BigInt(i)),
    value: byte(v),
    role: "source-row",
  }))
  .reverse();
const times = {
  load: 23,
  loaded: 37.5,
  add: 42,
  added: 52,
  store: 57,
  stored: 68,
  ret: 76,
  returned: 83,
};
export const experiment = {
  trace: {
    ...trace,
    runs: {
      single: {
        samples: trace.samples.map((s) => ({
          ...s,
          callsite: s.source,
          event:
            Object.entries(trace.events)
              .filter(([, i]) => i === s.index)
              .map(([name]) => name)
              .join(" / ") || s.instruction.asm,
        })),
      },
    },
  },
  duration: 0,
  cues: Object.entries(times).map(([event, time]) => ({
    run: "single",
    sampleIndex: trace.events[event],
    time,
    exact: false,
  })),
  scope:
    "同一次完整C执行的逐指令架构状态。取指、译码、ALU和控制路径是机制解释，不是硬件内部、时钟或总线的实测；略过的序言与局部读写均保留在实际停点中。",
};
