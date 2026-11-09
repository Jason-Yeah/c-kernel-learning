import trace from '../traces/byte-order.json' with {type:'json'};
import {validateByteOrder} from './validate-byte-order.js';
export const byteTrace = trace;
export const byteSummary = validateByteOrder(trace);
export const samples = trace.runs.single.samples;
export const at = event => {
  const s = samples.find(s => s.event === event);
  if (!s) throw new Error('Missing byte-order observation: '+event);
  return s;
};
export const ready = at('ready');
export const hx = n => '0x'+BigInt(n).toString(16).toUpperCase();
export const wordHex = n => '0x'+Number(n).toString(16).toUpperCase().padStart(8,'0');
export const byteHex = n => n.toString(16).toUpperCase().padStart(2,'0');
export function rows(name, s = ready) {
  const o = s.objects[name];
  return [3,2,1,0].map(i => ({key:String(i), label:`${name} + ${i}`, address:hx(BigInt(o.address)+BigInt(i)), value:byteHex(o.bytes[i]), role:name==='word'?'source-row':'target-row'}));
}
export function sourcePart(terms) {
  const lines = trace.provenance.source.content.split('\n');
  const numbers = terms.map(term => {
    const found = lines.flatMap((line,i)=>line.includes(term)?[i+1]:[]);
    if(found.length !== 1) throw new Error('Ambiguous measured C source: '+term);
    return found[0];
  });
  return {lines:numbers.map(n=>lines[n-1].trim()), numbers};
}
export const readTimes = [30.18,33.18,36.18,39.18];
export const encodeTimes = [51.08,53.08,55.08,57.08];
const times = [8,...readTimes,...encodeTimes,65.18,71.18,77.18];
export const experiment = {
  trace, duration:0,
  cues:samples.map((s,i)=>({run:'single',sampleIndex:s.index,time:times[i],exact:true})),
  scope:'同一次Linux x86-64小端运行的12个观察停点。word及地址持续不变；BE数组是本机C的显式编码，不是另一台大端CPU。箭头与教学速度不代表总线或真实耗时。'
};
