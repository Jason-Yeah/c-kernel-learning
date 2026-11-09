import buffer from "../traces/buffer-copy.json" with { type: "json" };
import recursion from "../traces/stack-recursion.json" with { type: "json" };
import cache from "../traces/cache-access.json" with { type: "json" };
import process from "../traces/process-lifecycle.json" with { type: "json" };
import forkMemory from "../traces/fork-memory.json" with { type: "json" };
import vm from "../traces/vm-access.json" with { type: "json" };
import { validateExperimentTrace, phaseOf } from "./validate-experiments.js";

export const experiments = {
  "buffer-copy": buffer,
  "stack-recursion": recursion,
  "cache-access": cache,
  "process-lifecycle": process,
  "fork-memory": forkMemory,
  "vm-access": vm,
};
for (const trace of Object.values(experiments)) validateExperimentTrace(trace);
export const experimentDurations = {
  "buffer-copy": 14,
  "stack-recursion": 12,
  "cache-access": 12,
  "process-lifecycle": 14,
  "fork-memory": 12,
  "vm-access": 14,
};
export const measuredBuffer = Object.fromEntries(
  Object.entries(buffer.runs).map(([key, run]) => {
    const ready = phaseOf(run, 1),
      done = phaseOf(run, 3),
      base = ready.fields;
    return [
      key,
      {
        run,
        ready,
        done,
        initial: base.memory,
        input: base.input,
        base: base.bufferAddress,
        inputAddress: base.inputAddress,
        returnBefore: base.savedReturn,
        returnAfter: done.fields.savedReturn,
        rbpOffset: Number(BigInt(base.rbpSlot) - BigInt(base.bufferAddress)),
        returnOffset: Number(
          BigInt(base.returnSlot) - BigInt(base.bufferAddress),
        ),
        writes: run.samples
          .filter((s) => s.phase === 2 && s.event !== "signal")
          .map((s) => ({
            index: s.fields.index,
            value: s.fields.input[s.fields.index],
            memory: s.fields.memory,
            returned: BigInt(s.fields.savedReturn),
            overflow: s.fields.index >= 8,
            address:
              "0x" +
              (BigInt(base.bufferAddress) + BigInt(s.fields.index))
                .toString(16)
                .toUpperCase(),
            sample: s,
          })),
      },
    ];
  }),
);
export const recursionEnters = recursion.runs.exhaust.samples.filter(
  (s) => s.phase === 1 && s.event !== "signal",
);
export const recursionFault = recursion.runs.exhaust.samples.find(
  (s) => s.event === "signal",
);
export const recursionDepth = recursionEnters.length;
export const recursionStackLimit = recursionEnters[0].fields.stackLimit;
export const recursionStride = Number(
  BigInt(recursionEnters[0].registers.rsp) -
    BigInt(recursionEnters[1].registers.rsp),
);
export const observedProcesses = Object.fromEntries(
  Object.entries(process.runs).map(([key, run]) => [
    key,
    {
      parent: phaseOf(run, 1).pid,
      child: phaseOf(run, 2).pid,
      status: phaseOf(run, 6).fields.waitStatus,
      wait: phaseOf(run, 6).fields.waitResult,
      output: run.stdout,
    },
  ]),
);
export const cacheReady = phaseOf(cache.runs.default, 1).fields;
export const cacheLoads = cache.runs.default.samples.filter(
  (s) => s.phase === 2,
);
export const vmObservations = Object.fromEntries(
  Object.entries(vm.runs).map(([key, run]) => [
    key,
    {
      before: phaseOf(run, 1),
      after: phaseOf(run, 2),
      fault: run.samples.find((s) => s.event === "signal"),
    },
  ]),
);
const hx = (n) => "0x" + Number(n).toString(16).toUpperCase().padStart(2, "0");
const findLine = (trace, term) => {
  const index = trace.provenance.source.content
    .split("\n")
    .findIndex((line) => line.includes(term));
  if (index < 0)
    throw new Error(`Measured source fragment missing: ${trace.id}: ${term}`);
  return index + 1;
};
function cue(trace, run, sample, title, readings, term, commands) {
  if (!sample)
    throw new Error(`Missing actual sample for ${trace.id}/${title}`);
  return {
    run,
    sampleIndex: sample.index,
    title,
    readings,
    focusLine: term
      ? findLine(trace, term)
      : sample.callsite.line || sample.source.line,
    commands,
  };
}
export function experimentalCues(id) {
  const trace = experiments[id];
  if (id === "buffer-copy") {
    const m = measuredBuffer.safe,
      first = m.writes[0].sample,
      last = m.writes.at(-1).sample;
    return [
      cue(
        trace,
        "safe",
        m.ready,
        "先确认两个独立对象",
        [
          ["input · 只读源地址", m.inputAddress, "source"],
          ["buf · 目标地址", m.base, "target"],
          ["buf 容量", `${m.ready.fields.capacity} bytes`],
        ],
        "_Alignas(16)",
        ["p/x input", "x/gx "],
      ),
      cue(
        trace,
        "safe",
        first,
        "第一个字节确实写入 buf",
        [
          ["input[0]", hx(first.fields.input[0]), "source"],
          ["buf[0]", hx(first.fields.memory[0]), "target"],
          ["当前写入索引", String(first.fields.index)],
        ],
        "buf[copy_index] =",
        ["p copy_index", "x/gx "],
      ),
      cue(
        trace,
        "safe",
        last,
        "最后的 00 也占一格",
        [
          ["input[7]", hx(last.fields.input[7]), "source"],
          ["buf[7]", hx(last.fields.memory[7]), "target"],
          ["保存的返回地址", last.fields.savedReturn],
        ],
        "if (input[copy_index]",
        ["p copy_index", "x/gx "],
      ),
      cue(
        trace,
        "safe",
        phaseOf(m.run, 4),
        "这次运行正常返回",
        [
          ["GDB 实际 PID", String(m.ready.pid)],
          ["返回地址保持", m.returnBefore],
          ["已观测写入数", String(m.writes.length)],
        ],
        'puts("normal return")',
        ["p/x input", "x/gx "],
      ),
    ];
  }
  if (id === "stack-recursion")
    return recursionEnters.slice(0, 4).map((s, i) =>
      cue(
        trace,
        "exhaust",
        s,
        `真实第 ${s.fields.depth} 层 f 调用`,
        [
          ["depth", String(s.fields.depth)],
          ["%rsp", s.registers.rsp],
          ["本层 block", `${s.fields.blockBytes} bytes`],
        ],
        i === 0 ? "volatile unsigned char block" : "child = f(depth + 1)",
        ["p depth", "p sizeof block"],
      ),
    );
  if (id === "cache-access") {
    const ready = phaseOf(trace.runs.default, 1);
    return [
      cue(
        trace,
        "default",
        ready,
        "先运行数组初始化",
        [
          ["memory 实际基址", cacheReady.base],
          ["实际数组长度", String(cacheReady.memory.length) + " bytes"],
          [
            "C 读取偏移（HEX）",
            cacheReady.offsets.map((n) => hx(n).slice(2)).join(" "),
          ],
        ],
        "memory[i] =",
        ["p/x &memory[0]"],
      ),
      ...[0, 1, 7].map((i) => {
        const s = cacheLoads[i],
          f = s.fields;
        return cue(
          trace,
          "default",
          s,
          `第 ${i + 1} 次实际读取`,
          [
            ["target · 真实 VA", f.target],
            ["offset · 数组偏移", hx(f.offset)],
            ["读取值／AL", hx(f.value)],
          ],
          "value = *target",
          ["p/x target", "p/x value"],
        );
      }),
    ];
  }
  if (id === "process-lifecycle") {
    const run = trace.runs.early;
    return [1, 2, 3, 4]
      .map((phase) => {
        const s = phaseOf(run, phase),
          f = s.fields;
        const title = {
          1: "父进程：fork 返回子 PID",
          2: "子进程：fork 返回 0",
          3: "exec 后仍是同一个子 PID",
          4: "父进程停在 waitpid 之前",
        }[phase];
        const values =
          phase === 4
            ? [
                ["当前 PID", String(s.pid)],
                ["child_id", String(f.childPid)],
                ["停点性质", "waitpid 调用前"],
              ]
            : [
                ["当前 PID", String(s.pid)],
                [
                  "fork 返回值",
                  phase === 3 ? "exec 后全局重新初始化" : String(f.forkResult),
                ],
                ["child_id", String(f.childPid)],
              ];
        return cue(
          trace,
          "early",
          s,
          title,
          values,
          phase === 1
            ? "fork_result = fork()"
            : phase === 2
              ? "if (fork_result == 0)"
              : phase === 3
                ? "if (argc == 5"
                : "wait_result = waitpid",
          ["p fork_result", "p child_id"],
        );
      })
      .sort((a, b) => a.sampleIndex - b.sampleIndex);
  }
  if (id === "fork-memory")
    return [1, 2, 3, 4].map((phase) => {
      const s = phaseOf(trace.runs.default, phase),
        f = s.fields;
      return cue(
        trace,
        "default",
        s,
        {
          1: "父进程先记录原值",
          2: "子进程看到相同虚拟地址",
          3: "子进程已修改六类对象",
          4: "父进程经管道同步后读取",
        }[phase],
        [
          ["PID", String(s.pid)],
          ["private_page · 私有值", `${f.privateValue} @ ${f.privateAddress}`],
          ["shared_page · 共享值", `${f.sharedValue} @ ${f.sharedAddress}`],
        ],
        phase <= 2
          ? "*shared_page = 50"
          : phase === 3
            ? "*shared_page = 500"
            : "if (waitpid(child_id,",
        ["p *private_page", "p *shared_page"],
      );
    });
  if (id === "vm-access")
    return [
      ["resident-a", 1],
      ["resident-a", 2],
      ["cold", 1],
      ["cold", 2],
    ].map(([key, phase]) => {
      const s = phaseOf(trace.runs[key], phase),
        f = s.fields;
      return cue(
        trace,
        key,
        s,
        `${key === "cold" ? "独立运行：首次访问冷页" : "独立运行：已驻留页"} · ${phase === 1 ? "访问前" : "访问后"}`,
        [
          ["target · 实际 VA", f.target],
          ["OS pagemap present", String(Number(f.page.present))],
          [
            "page_size / 已读值",
            phase === 1 ? `${f.pageSize} bytes` : hx(f.value),
          ],
        ],
        phase === 1
          ? key === "cold"
            ? "target = base + page_size + 0x2D;"
            : "target = base + 0x2D;"
          : "value = *target;",
        ["p/x target", "p page_size"],
      );
    });
  throw new Error(`No experimental storyboard for ${id}`);
}
export function sourceExcerpt(trace, cue, maxRows = 7) {
  const lines = trace.provenance.source.content.split("\n");
  const cleaned = (line) => line.replace(/\s*\/\*.*?\*\//g, "").trimEnd();
  // The full, unmodified instrumented program stays in the workbench. In the
  // film omit observer-only statements, keeping original line numbers intact.
  const valid = lines
    .map((line, i) => ({ text: cleaned(line), index: i }))
    .filter(
      (row) =>
        row.text.trim() &&
        !/__asm__|trace_phase\s*=|checkpoint\(\d+\)/.test(row.text),
    );
  let position = valid.findIndex((row) => row.index >= cue.focusLine - 1);
  if (position < 0) position = valid.length - 1;
  const selected = [valid[position]];
  let left = position - 1,
    right = position + 1;
  const cost = (rows) =>
    rows.reduce(
      (sum, row) => sum + Math.max(1, Math.ceil(row.text.length / 44)),
      0,
    );
  while (selected.length < 5) {
    if (left >= 0 && cost([...selected, valid[left]]) <= maxRows)
      selected.push(valid[left--]);
    else if (
      right < valid.length &&
      cost([...selected, valid[right]]) <= maxRows
    )
      selected.push(valid[right++]);
    else break;
  }
  selected.sort((a, b) => a.index - b.index);
  return {
    lines: selected.map((row) => row.text),
    numbers: selected.map((row) => row.index + 1),
    focus: cue.focusLine,
  };
}
export function cueCommands(trace, cue) {
  const sample = trace.runs[cue.run].samples[cue.sampleIndex];
  return cue.commands
    .map((prefix) => {
      const command = sample.commands.find((item) =>
        item.command.startsWith(prefix),
      );
      if (!command)
        throw new Error(
          `Actual GDB command missing: ${trace.id}/${cue.run} #${cue.sampleIndex}: ${prefix}`,
        );
      return command;
    })
    .map((item) => `(gdb) ${item.command}\n${item.output}`)
    .join("\n");
}
