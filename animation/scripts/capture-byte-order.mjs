import { spawnSync } from 'node:child_process';
import { readFile, writeFile, mkdir, rename } from 'node:fs/promises';
import { createHash } from 'node:crypto';
import { resolve } from 'node:path';
import { validateByteOrder } from '../models/validate-byte-order.js';
const root = resolve(import.meta.dirname, '..'), dir = '.trace-build/byte-order';
const flags = ['-std=c11','-O0','-g3','-fPIE','-pie','-fno-omit-frame-pointer','-fno-optimize-sibling-calls','-Wall','-Wextra','-Wpedantic'];
const sourcePath = 'experiments/byte-order.c', compiler = process.env.CC || 'gcc', binaryPath = dir + '/program';
function run(exe, args) {
  const p = spawnSync(exe, args, {cwd: root, encoding:'utf8', timeout:35000, maxBuffer:16*1024*1024});
  if (p.error || p.status !== 0) throw new Error(`${exe} failed, trace not published\n${p.error || ''}\n${p.stdout}\n${p.stderr}`);
  return p.stdout;
}
const sha = b => createHash('sha256').update(b).digest('hex');
await mkdir(resolve(root, dir), {recursive:true});
run(compiler, [...flags, sourcePath, '-o', binaryPath]);
await writeFile(resolve(root, dir, 'session.log'), run('gdb', ['-nx','-q','-batch','-x','scripts/gdb-byte-order.py']));
const raw = JSON.parse(await readFile(resolve(root, dir, 'raw.json'), 'utf8'));
const content = await readFile(resolve(root, sourcePath), 'utf8'), binary = await readFile(resolve(root, binaryPath));
const trace = {schemaVersion:1, id:'byte-order', provenance:{
  kind:'gdb-measured', capturedAt:new Date().toISOString(),
  source:{path:sourcePath, content, sha256:sha(content)},
  binary:{path:'traces/byte-order/program.elf',sha256:sha(binary)},
  compiler:run(compiler,['--version']).split('\n')[0],
  compilerCommand:[compiler,...flags,sourcePath,'-o',binaryPath].join(' '), flags,
  gdb:'GNU gdb '+raw.gdb, architecture:raw.architecture,
  kernelASLR:(await readFile('/proc/sys/kernel/randomize_va_space','utf8')).trim(), debuggerASLR:raw.randomization,
  scope:'Actual native x86-64 little-endian word, four byte reads, explicit BE encoding array and memcpy/shift results. Not a big-endian CPU run; arrows and teaching time are explanatory.'
}, runs:{single:{samples:raw.samples}}, exitCode:raw.exitCode, stdout:raw.stdout};
const result = validateByteOrder(trace);
await mkdir(resolve(root,'traces/byte-order'),{recursive:true});
await writeFile(resolve(root,'traces/byte-order/program.elf'),binary);
const target = resolve(root,'traces/byte-order.json');
await writeFile(target+'.tmp',JSON.stringify(trace,null,2)+'\n');
await rename(target+'.tmp',target);
await writeFile(resolve(root,'traces/byte-order-gdb.txt'), `Captured: ${trace.provenance.capturedAt}\nSource SHA256: ${trace.provenance.source.sha256}\nELF SHA256: ${trace.provenance.binary.sha256}\n${trace.provenance.compilerCommand}\n\n${raw.gdbText}`);
console.log(`PASS byte order: ${result.samples} actual stops, native bytes ${result.little}, explicit BE bytes ${result.big}, normal exit0`);
