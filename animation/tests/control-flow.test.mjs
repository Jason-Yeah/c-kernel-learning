import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {parseELF,elfSymbol,bytesAtVA} from '../models/elf.js';
import {validateControlFlow,procFields} from '../models/validate-control-flow.js';
const t=JSON.parse(await readFile(new URL('../traces/control-flow.json',import.meta.url),'utf8'));
const at=(r,e)=>t.runs[r].samples.find(s=>s.event===e),sha=b=>createHash('sha256').update(b).digest('hex');
test('control flow: reviewed complete source / binary / exact SYSCALL and fault bytes from ELF',async()=>{
 const source=await readFile(new URL('../'+t.provenance.source.path,import.meta.url));const binary=await readFile(new URL('../'+t.provenance.binary.path,import.meta.url));
 assert.equal(source.toString(),t.provenance.source.content);assert.equal(sha(source),t.provenance.source.sha256);assert.equal(sha(binary),t.provenance.binary.sha256);
 const elf=parseELF(binary);
 for(const[run,label,next,event,bytes]of[['inspect','ec_syscall','ec_syscall_after','syscall-before',[15,5]],['fault','ec_fault','ec_fault_after','fault-before',[138,7]]]){
  const sym=elfSymbol(elf,label),n=elfSymbol(elf,next),s=at(run,event);assert.deepEqual(bytesAtVA(binary,elf,sym.value,2),bytes);assert.equal(BigInt(n.value)-BigInt(sym.value),2n);
  const bias=BigInt(s.registers.rip)-BigInt(sym.value);
  for(const state of t.runs[run].samples){assert.deepEqual(bytesAtVA(binary,elf,BigInt(state.registers.rip)-bias,state.instruction.length),state.instruction.bytes);}
 }
 assert.deepEqual(validateControlFlow(t),{gdbRuns:2,stops:10,osRuns:1});
});
test('control flow: all raw commands, registers, initialized fields and signal notifications independently match',async()=>{
 const log=await readFile(new URL('../traces/control-flow-gdb.txt',import.meta.url),'utf8');
 assert.ok(log.includes(t.gdbSession));assert.ok(log.includes('set print frame-arguments none'));assert.ok(!/fault_load \(p=0x/.test(t.gdbSession));
 for(const run of Object.values(t.runs))for(const s of run.samples){
  for(const c of s.commands)assert.ok(log.includes('(gdb) '+c.command+'\n'+c.output));
  for(const[name,value]of Object.entries(s.registers)){const match=s.commands[0].output.match(new RegExp('^'+name+'\\s+(0x[0-9a-f]+)','im'));assert.ok(match);assert.equal(BigInt(match[1]),BigInt(value));}
  for(const[name,value]of Object.entries(s.fields)){const c=s.commands.find(c=>c.command==='p/d '+name);assert.equal(Number(c.output.match(/=\s*(-?\d+)/)[1]),value);}
 }
});
test('control flow: CALL versus SYSCALL uses actual user-state continuations, not fabricated kernel RIP',()=>{
 const e=at('inspect','function-entry'),r=at('inspect','function-return'),a=at('inspect','syscall-before'),b=at('inspect','syscall-after');
 assert.equal(BigInt(e.registers.rdi)+BigInt(e.registers.rsi),BigInt(r.registers.rax));assert.equal(r.registers.rip,e.returnAddress);
 assert.equal(BigInt(r.registers.rsp),BigInt(e.registers.rsp)+8n);assert.equal(a.registers.rsp,b.registers.rsp);
 assert.equal(BigInt(a.registers.rax),BigInt(a.fields.syscall_number));assert.equal(BigInt(b.registers.rax),BigInt(b.pid));assert.equal(BigInt(b.registers.rip),BigInt(a.registers.rip)+2n);assert.equal(b.registers.rcx,b.registers.rip);
 assert.ok(Object.values(t.runs).every(r=>r.samples.every(s=>(BigInt(s.registers.cs)&3n)===3n)));
});
test('control flow: protected MOV has no returned byte, no debugger target read and actual ACCERR address',()=>{
 const a=at('fault','fault-before'),b=at('fault','fault-signal');assert.equal(a.registers.rip,b.registers.rip);assert.equal(b.signal.address,a.guardAddress);assert.equal(b.signal.code,2);
 assert.equal(t.runs.fault.outcome.kind,'signal');assert.equal(b.fields.fault_read_complete,0);assert.equal(at('fault','fault-ready').guardByte,165);
 for(const s of[a,b]){assert.ok(!s.commands.some(c=>c.command==='x/1bx guard_page'));assert.ok(!('guardByte'in s));}
});
test('control flow: independent non-GDB read blocks before release, real affinity, byte and rusage / wait evidence',async()=>{
 const o=t.os.blocking,r=o.records[0],h=o.records[1],done=o.records[2],files=o.beforeRelease.files;
 const p=procFields(files.find(f=>f.path===`/proc/${r.pid}/status`).content),child=procFields(files.find(f=>f.path===`/proc/${h.pid}/status`).content);
 assert.ok(p.State.startsWith('S')&&child.State.startsWith('S'));assert.equal(Number(child.PPid),r.pid);assert.equal(p.Cpus_allowed_list,String(r.cpu));assert.equal(child.Cpus_allowed_list,String(h.cpu));
 const args=files.find(f=>f.path.endsWith('/syscall')).content.trim().split(/\s+/);assert.equal(Number(args[0]),0);assert.equal(BigInt(args[2]),BigInt(r.buffer));assert.equal(BigInt(args[3]),1n);
 assert.equal(r.beforeByte,204);assert.equal(h.byte,done.byte);assert.equal(done.readResult,1);assert.equal(done.byte,75);assert.ok(done.nvAfter>done.nvBefore);assert.equal(done.waited,h.pid);assert.equal(done.status,0);
 const report=await readFile(new URL('../traces/control-flow/blocking-os.txt',import.meta.url),'utf8');assert.ok(report.includes(o.stdout));for(const f of files)assert.ok(report.includes(f.operation+' '+f.path+'\n'+f.content));
});
test('control flow: corrupt identity, bytes, architectural state, fault or OS evidence is rejected',()=>{
 for(const mutate of[
  t=>t.runs.inspect.samples.pop(),t=>t.runs.inspect.samples[4].instruction.bytes=[205,128],
  t=>t.runs.inspect.samples[5].registers.rip=t.runs.inspect.samples[4].registers.rip,
  t=>t.runs.inspect.samples[0].fields.observed_pid=7,t=>t.runs.inspect.samples[1].returnAddress='0xBAD',
  t=>t.runs.fault.samples[2].signal.address='0xBAD',t=>t.runs.fault.samples[2].fields.fault_read_complete=1,
  t=>t.runs.fault.samples[1].guardByte=165,t=>t.runs.fault.outcome={kind:'exit',code:0},
  t=>t.os.blocking.records[2].byte=204,t=>t.os.blocking.records[2].nvAfter=t.os.blocking.records[2].nvBefore,
  t=>t.os.blocking.beforeRelease.files.pop(),t=>t.gdbSession='',t=>t.runs.inspect.samples[3].commands.pop(),
 ]){const bad=structuredClone(t);mutate(bad);assert.throws(()=>validateControlFlow(bad),/^Error: Control-flow evidence rejected:/);}
});
