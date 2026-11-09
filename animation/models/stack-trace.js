import trace from "../traces/stack-call.json" with { type: "json" };
import {
  eventOf,
  variableOf,
  wordAt,
  validateStackTrace,
} from "./validate-stack-trace.js";
validateStackTrace(trace);
export { trace };
export const measured = Object.fromEntries(
  Object.keys(trace.events).map((name) => [name, eventOf(trace, name)]),
);
export const reg = (name, register) => measured[name].registers[register];
export const local = (name, fn, variable) =>
  variableOf(measured[name], fn, variable);
export const decimal = (value) => Number(BigInt(value));
export const allocation = (fn) =>
  Number(
    BigInt(reg(`${fn}_frame_ready`, "rbp")) -
      BigInt(reg(`${fn}_frame_ready`, "rsp")),
  );
export const saved = (fn, kind) => {
  const sample = measured[`${fn}_frame_ready`];
  const address = BigInt(sample.registers.rbp) + (kind === "return" ? 8n : 0n);
  return wordAt(sample, address);
};
export const sourceLines = (fn) =>
  trace.functions[fn].source.map((row) =>
    row.text.replace(/\s*\/\* TRACE_\w+ \*\//g, "").trimEnd(),
  );
export const sourceLineNumbers = (fn) =>
  trace.functions[fn].source.map((row) => row.line);
export const sourceTitle = (fn) =>
  `stack-call.c:${trace.functions[fn].source[0].line}–${trace.functions[fn].source.at(-1).line} / 实验节选`;
export const prologue = (fn) =>
  trace.functions[fn].instructions
    .filter((i) => /^(push\s+%rbp|mov\s+%rsp,%rbp|sub\s+\$.*,%rsp)/.test(i.asm))
    .map((i) => i.asm);
export const epilogue = (fn) => {
  const instructions = trace.functions[fn].instructions;
  const index = instructions.findIndex((i) => i.asm.startsWith("ret"));
  return instructions.slice(index - 1, index + 1).map((i) => i.asm);
};
