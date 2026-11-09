import { build } from "esbuild";
import { cp, mkdir, rm, writeFile } from "node:fs/promises";
import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { films, chapters } from "../shared/catalog.js";
import { generatePages } from "./pages.mjs";
const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
await generatePages();
const dist = resolve(root, "dist");
await rm(dist, { recursive: true, force: true });
await mkdir(dist, { recursive: true });
await build({
  absWorkingDir: root,
  entryPoints: films.map((f) => `chapter${f.chapter}/${f.file}.js`),
  outdir: "dist",
  outbase: ".",
  bundle: true,
  splitting: true,
  format: "esm",
  target: ["es2022"],
  minify: true,
  chunkNames: "shared/chunks/[name]-[hash]",
  legalComments: "linked",
  logLevel: "info",
});
for (const file of [
  "index.html",
  "shared/film.css",
  "shared/diagrams.css",
  "shared/evidence.css",
  "shared/os-basics.css",
  "shared/instruction-cycle.css",
  "shared/byte-order.css",
  "shared/integer-bits.css",
  "shared/control-flow.css",
  "experiments/control-flow.c",
  "traces/control-flow.json",
  "traces/control-flow-gdb.txt",
  "traces/control-flow",
  "chapter8/exception-control.md",
  "chapter8/guide.html",
  "experiments/integer-bits.c",
  "traces/integer-bits.json",
  "traces/integer-bits-gdb.txt",
  "traces/integer-bits",
  "chapter2/integers.md",
  "chapter2/guide.html",
  "experiments/byte-order.c",
  "traces/byte-order.json",
  "traces/byte-order-gdb.txt",
  "traces/byte-order",
  "chapter2/byte-order.md",
  "experiments/instruction-cycle.c",
  "traces/instruction-cycle.json",
  "traces/instruction-cycle-gdb.txt",
  "traces/instruction-cycle",
  "chapter1/instruction-cycle.md",
  "experiments/os-main.c",
  "experiments/os-math.c",
  "experiments/os-demo.h",
  "traces/os-basics.json",
  "traces/os-basics-gdb.txt",
  "traces/os-basics",
  "chapter0/os-basics.md",
  "chapter0/guide.html",
  ...[
    "stack-call",
    "buffer-copy",
    "stack-recursion",
    "cache-access",
    "process-lifecycle",
    "fork-memory",
    "vm-access",
  ].flatMap((id) => [
    `experiments/${id}.c`,
    `traces/${id}.json`,
    `traces/${id}-gdb.txt`,
  ]),
  ...chapters.map((c) => `chapter${c.number}/index.html`),
  ...films.map((f) => `chapter${f.chapter}/${f.file}.html`),
  "chapter8/fork-memory-demo.c",
]) {
  const target = resolve(dist, file);
  await mkdir(dirname(target), { recursive: true });
  await cp(resolve(root, file), target, { recursive: true });
}
await writeFile(resolve(dist, ".nojekyll"), "");
console.log(
  "Static site ready: dist/ — HTML, CSS and bundled GSAP; no node_modules or server required.",
);
