import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {parseELF,elfSymbol,bytesAtVA} from '../models/elf.js';
import {validateIntegerBits} from '../models/validate-integer-bits.js';
const t=JSON.parse(await readFile(new URL('../traces/integer-bits.json',import.meta.url),'utf8'));
const ss=t.runs.single.samples, sample=e=>ss.find(s=>s.event===e), sha=b=>createHash('sha256').update(b).digest('hex');
test('integer bits: complete source / ELF symbols / initial bytes / actual one-run bias',async()=>{
 const source=await readFile(new URL('../'+t.provenance.source.path,import.meta.url));
 const bytes=await readFile(new URL('../'+t.provenance.binary.path,import.meta.url));
 assert.equal(source.toString(),t.provenance.source.content);assert.equal(sha(source),t.provenance.source.sha256);assert.equal(sha(bytes),t.provenance.binary.sha256);
 const elf=parseELF(bytes),bias=BigInt(ss[0].objects.bits.address)-BigInt(elfSymbol(elf,'bits').value);
 for(const [name,o]of Object.entries(ss[0].objects)){
  const sym=elfSymbol(elf,name);assert.equal(sym.size,o.width/8);assert.equal(BigInt(o.address)-BigInt(sym.value),bias);
  const section=elf.sections[sym.sectionIndex];
  if(section.type===8)assert.ok(o.bytes.every(b=>b===0));else assert.deepEqual(bytesAtVA(bytes,elf,sym.value,sym.size),o.bytes);
 }
 assert.equal(validateIntegerBits(t).samples,17);
});
test('integer bits: every typed object, original command, register and C checkpoint is independently present',async()=>{
 const log=await readFile(new URL('../traces/integer-bits-gdb.txt',import.meta.url),'utf8');
 for(const item of t.types)assert.ok(log.includes('(gdb) '+item.evidence.command+'\n'+item.evidence.output));
 for(const s of ss){
  for(const c of s.commands)assert.ok(log.includes('(gdb) '+c.command+'\n'+c.output));
  assert.ok(t.provenance.source.content.split('\n')[s.callsite.line-1].includes(`checkpoint(${s.phase})`));
  for(const [r,v]of Object.entries(s.registers)){const m=s.commands[0].output.match(new RegExp('^'+r+'\\s+(0x[0-9a-f]+)','im'));assert.ok(m);assert.equal(BigInt(m[1]),BigInt(v));}
  for(const [name,o]of Object.entries(s.objects)){
   const c=s.commands.find(c=>c.command===(o.signed?'p/d ':'p/u ')+name);assert.equal(Number(c.output.match(/=\s*(-?\d+)/)[1]),s.fields[name]);
   const m=s.commands.find(c=>c.command===`x/${o.width/8}bx &${name}`).output;
   assert.equal(BigInt(m.match(/^0x[0-9a-f]+/i)[0]),BigInt(o.address));assert.deepEqual([...m.split(':').slice(1).join(':').matchAll(/\b0x([0-9a-f]{2})\b/gi)].map(m=>parseInt(m[1],16)),o.bytes);
  }
 }
});
test('integer bits: six operations, promotion and full 32-bit results (not only low byte)',()=>{
 const a=BigInt(ss[0].fields.bits),b=BigInt(ss[0].fields.mask),u32=n=>Number(BigInt.asUintN(32,n));
 for(const[e,key,expected]of[['and','r_and',a&b],['or','r_or',a|b],['xor','r_xor',a^b],['not','r_not',~a],['left','r_left',a<<2n],['right','r_right',a>>2n]])assert.equal(sample(e).fields[key],u32(expected));
 const p=sample('promotion').fields;assert.equal(p.promoted_not,-Number(a)-1);assert.equal(p.narrow_not,Number(BigInt.asUintN(8,BigInt(p.promoted_not))));assert.equal(p.small_shift,Number(a)*256);
 assert.equal(sample('and').fields.r_logical,1);assert.notEqual(sample('and').fields.r_and,sample('and').fields.r_logical);
 assert.equal(sample('left').fields.r_left>>>8,2);assert.equal(sample('shift-guard').fields.guarded_shift,0);
});
test('integer bits: signed weights, sign/zero extension, narrowing and modular addition',()=>{
 const s=sample('representation');assert.deepEqual(s.objects.u8.bytes,s.objects.s8.bytes);
 const byte=s.objects.u8.bytes[0],negative=byte-256;assert.equal(s.fields.u8,byte);assert.equal(s.fields.s8,negative);
 const n=sample('negate8').fields;assert.equal(n.negated8,(~s.fields.magnitude+1)&255);
 const e=sample('extend');assert.deepEqual(e.objects.zero_extended.bytes,[byte,0]);assert.deepEqual(e.objects.sign_extended.bytes,[byte,255]);assert.equal(e.fields.sign_extended,negative);
 const c=sample('truncate').fields;assert.equal(c.truncated,c.wide_unsigned%256);
 const w=sample('unsigned-wrap').fields;assert.equal(w.wrapped,Number(BigInt.asUintN(32,BigInt(w.wrap_lhs)+1n)));assert.equal(w.promoted_256,256);assert.equal(w.wrapped8,0);
});
test('integer bits: rejected signed overflow has no fictitious wrapped result; actual UBSan run clean',async()=>{
 const s=sample('signed-guard').fields;assert.equal(s.wide_sum,s.signed_lhs+1);assert.ok(s.wide_sum>2147483647);assert.equal(s.signed_rejected,1);assert.equal(s.signed_result,0);
 const p=sample('promoted-add').fields;assert.equal(p.promoted_add,128);assert.equal(p.promoted_negmin,128);
 const report=await readFile(new URL('../traces/integer-bits/sanitizer.txt',import.meta.url),'utf8');assert.ok(report.includes(t.sanitizer.command));assert.ok(report.includes('exitCode=0'));assert.equal(t.sanitizer.stderr,'');assert.equal(t.sanitizer.stdout,t.stdout);
});
test('integer bits: corrupt state, bytes, types, lifetime, guard or missing raw evidence is rejected',()=>{
 for(const mutate of[
  t=>t.runs.single.samples.pop(),t=>t.runs.single.samples[0].objects.u8.bytes[0]=0,
  t=>t.runs.single.samples[4].fields.r_not=73,t=>t.runs.single.samples[6].fields.r_left=216,
  t=>t.runs.single.samples[1].fields.promoted_add=128,t=>t.runs.single.samples[12].objects.sign_extended.bytes[1]=0,
  t=>t.runs.single.samples[13].objects.truncated.address='0x12',t=>t.runs.single.samples[15].fields.signed_result=-2147483648,
  t=>t.runs.single.samples[16].commands.pop(),t=>t.types[3].width=8,t=>t.types[3].signed=false,
  t=>t.runs.single.samples[2].objects.bits.width=8,t=>t.runs.single.samples[0].objects.small.type='int',
  t=>t.runs.single.samples[3].registers.rip='0xBAD',t=>t.types[0].type='int',
  t=>t.sanitizer.stderr='runtime error',t=>t.exitCode=1,
 ]){const bad=structuredClone(t);mutate(bad);assert.throws(()=>validateIntegerBits(bad),/^Error: Integer-bits evidence rejected:/);}
});
