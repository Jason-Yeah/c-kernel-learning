import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {validateByteOrder} from '../models/validate-byte-order.js';
import {parseELF,elfSymbol,bytesAtVA} from '../models/elf.js';
const t=JSON.parse(await readFile(new URL('../traces/byte-order.json',import.meta.url),'utf8'));
const samples=t.runs.single.samples, sha=b=>createHash('sha256').update(b).digest('hex');
test('byte order: complete measured source / actual ELF / fixed initial data',async()=>{
  const source=await readFile(new URL('../'+t.provenance.source.path,import.meta.url));
  const binary=await readFile(new URL('../'+t.provenance.binary.path,import.meta.url));
  assert.equal(source.toString(),t.provenance.source.content);
  assert.equal(sha(source),t.provenance.source.sha256); assert.equal(sha(binary),t.provenance.binary.sha256);
  const elf=parseELF(binary), word=elfSymbol(elf,'word'), be=elfSymbol(elf,'be_encoded');
  const initial=bytesAtVA(binary,elf,word.value,4), cc=bytesAtVA(binary,elf,be.value,4);
  assert.deepEqual(initial,samples[0].objects.word.bytes); assert.deepEqual(cc,[204,204,204,204]);
  assert.equal(new DataView(Uint8Array.from(initial).buffer).getUint32(0,true),samples[0].fields.word);
  const bias=BigInt(samples[0].objects.word.address)-BigInt(word.value);
  assert.equal(BigInt(samples[0].objects.be_encoded.address)-BigInt(be.value),bias);
  assert.equal(validateByteOrder(t).samples,samples.length);
});
test('byte order: every raw GDB byte, scalar and register matches the structured record',async()=>{
  const log=await readFile(new URL('../traces/byte-order-gdb.txt',import.meta.url),'utf8');
  for(const s of samples){
    for(const c of s.commands) assert.ok(log.includes('(gdb) '+c.command+'\n'+c.output));
    for(const [name,command] of [['word','x/4bx &word'],['be_encoded','x/4bx be_encoded']]){
      const output=s.commands.find(c=>c.command===command).output;
      assert.equal(BigInt(output.match(/^0x[0-9a-f]+/i)[0]),BigInt(s.objects[name].address));
      assert.deepEqual([...output.matchAll(/\b0x([\da-f]{1,2})\b/gi)].map(m=>parseInt(m[1],16)),s.objects[name].bytes);
    }
    const regs=s.commands[0].output;
    for(const [name,value] of Object.entries(s.registers)){
      const match=regs.match(new RegExp('^'+name+'\\s+(0x[\\da-f]+)','im'));
      assert.ok(match);assert.equal(BigInt(match[1]),BigInt(value));
    }
    for(const name of ['word','native_read','raw_read','decoded_be','probe_byte']){
      const output=s.commands.find(c=>c.command==='p/x '+name).output;
      assert.equal(Number(output.match(/=\s*(0x[\da-f]+)/i)[1]),s.fields[name]);
    }
    const line=t.provenance.source.content.split('\n')[s.callsite.line-1];
    assert.ok(line.includes(`checkpoint(${s.phase})`));
  }
});
test('byte order: unchanged source, four exact commits and correct interpretation',()=>{
  const ready=samples[0]; let before=[204,204,204,204];
  for(const s of samples){
    assert.deepEqual(s.objects.word,ready.objects.word);
    if(s.phase===2)assert.equal(s.fields.probe_byte,ready.objects.word.bytes[s.fields.read_index]);
    if(s.phase===3)before[s.fields.encode_index]=ready.objects.word.bytes[3-s.fields.encode_index];
    assert.deepEqual(s.objects.be_encoded.bytes,before);
  }
  assert.equal(samples[9].fields.native_read,ready.fields.word);
  const raw=samples[10].objects.be_encoded.bytes.reduce((v,b,i)=>v+b*256**i,0);
  const decoded=samples[11].objects.be_encoded.bytes.reduce((v,b)=>v*256+b,0);
  assert.equal(samples[10].fields.raw_read,raw);assert.equal(samples[11].fields.decoded_be,decoded);
  assert.notEqual(raw,decoded);
});
test('byte order: corrupt or missing evidence fails closed',()=>{
  for(const mutate of [
    t=>t.runs.single.samples.pop(),
    t=>t.runs.single.samples[4].objects.word.bytes[0]=0,
    t=>t.runs.single.samples[6].objects.be_encoded.bytes[3]=0,
    t=>t.runs.single.samples[2].objects.word.address='0x42',
    t=>t.runs.single.samples[1].fields.probe_byte=255,
    t=>t.runs.single.samples[3].commands.pop(),
    t=>t.runs.single.samples[5].phase=1,
    t=>t.runs.single.samples[1].fields.native_read=5,
    t=>t.runs.single.samples[10].fields.raw_read=1,
    t=>t.runs.single.samples[11].fields.decoded_be=1,
    t=>t.exitCode=1,
  ]){
    const bad=structuredClone(t); mutate(bad);assert.throws(()=>validateByteOrder(bad),/^Error: Byte-order evidence rejected:/);
  }
});
