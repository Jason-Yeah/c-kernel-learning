import assert from "node:assert/strict";
import { mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";
const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const dir = await mkdtemp(resolve(tmpdir(), "csapp-fork-test-"));
let runs = 0;
try {
  for (const optimization of ["-O0", "-O2"]) {
    const binary = resolve(dir, `fork-${optimization.slice(1)}`);
    const compile = spawnSync(
      "cc",
      [
        "-std=c11",
        optimization,
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-Werror",
        resolve(root, "chapter8/fork-memory-demo.c"),
        "-o",
        binary,
      ],
      { encoding: "utf8" },
    );
    assert.equal(compile.status, 0, compile.stderr);
    for (const args of [[], [], [], ["--maps"]]) {
      const result = spawnSync(binary, args, {
        encoding: "utf8",
        timeout: 10000,
      });
      assert.equal(result.status, 0, result.stderr);
      assert.match(result.stdout, /PASS: private values stay isolated/);
      const snapshots = [
        ...result.stdout.matchAll(
          /SNAP (\w+) pid=(\d+) ppid=(\d+)\n([\s\S]*?)(?=\nSNAP |\nPASS:|$)/g,
        ),
      ].map((m) => ({
        name: m[1],
        pid: m[2],
        ppid: m[3],
        vars: Object.fromEntries(
          [...m[4].matchAll(/VAR (\w+)\s+addr=(\S+) value=(\d+)/g)].map((v) => [
            v[1],
            { address: v[2], value: Number(v[3]) },
          ]),
        ),
      }));
      assert.deepEqual(
        snapshots.map((s) => s.name),
        ["PARENT_BEFORE", "CHILD_BEFORE", "CHILD_AFTER", "PARENT_AFTER"],
      );
      const before = {
        data: 10,
        bss: 0,
        heap: 30,
        stack: 20,
        private: 40,
        shared: 50,
      };
      const after = {
        data: 100,
        bss: 200,
        heap: 300,
        stack: 200,
        private: 400,
        shared: 500,
      };
      for (const [i, s] of snapshots.entries()) {
        const values = Object.fromEntries(
          Object.entries(s.vars).map(([key, v]) => [key, v.value]),
        );
        assert.deepEqual(
          values,
          i < 2 ? before : i === 2 ? after : { ...before, shared: 500 },
        );
        for (const key of Object.keys(before))
          assert.equal(s.vars[key].address, snapshots[0].vars[key].address);
      }
      assert.equal(snapshots[1].ppid, snapshots[0].pid);
      assert.notEqual(snapshots[0].pid, snapshots[1].pid);
      if (args.length) {
        assert.match(result.stdout, /MAP private\s+[^\n]*rw-p/);
        assert.match(result.stdout, /MAP shared\s+[^\n]*rw-s/);
      }
      runs++;
    }
  }
  console.log(
    `PASS: C example, -O0/-O2, ${runs} executions, synchronization, values, addresses, maps and child reaping`,
  );
} finally {
  await rm(dir, { recursive: true, force: true });
}
