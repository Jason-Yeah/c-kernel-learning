import trace from '../traces/integer-bits.json' with {type:'json'};
import {validateIntegerBits} from './validate-integer-bits.js';
export const integerTrace=trace;
export const integerSummary=validateIntegerBits(trace);
export const samples=trace.runs.single.samples;
export function at(event){const s=samples.find(s=>s.event===event);if(!s)throw Error('Missing integer-bits observation: '+event);return s;}
export const ready=at('ready');
export const value=(event,name)=>at(event).fields[name];
export const unsigned=(n,width)=>BigInt.asUintN(width,BigInt(n));
export const binary=(n,width=8)=>unsigned(n,width).toString(2).padStart(width,'0');
export const hex=(n,width=32)=>'0x'+unsigned(n,width).toString(16).toUpperCase().padStart(width/4,'0');
export function sourcePart(terms){const lines=trace.provenance.source.content.split('\n');const numbers=terms.map(term=>{const matches=lines.flatMap((line,i)=>line.includes(term)?[i+1]:[]);if(matches.length!==1)throw Error('Ambiguous measured integer C: '+term);return matches[0];});return{lines:numbers.map(i=>lines[i-1].trim()),numbers};}
export const bitCues={ready:8,and:24,or:38,xor:46,not:56,promotion:65,left:77,right:85,'shift-guard':93};
export const integerCues={ready:8,representation:26,invert8:37,negate8:42,extend:59,truncate:76,'unsigned-wrap':94,'signed-guard':109,'promoted-add':114};
export function experimentFor(id){const cues=id==='bitwise'?bitCues:integerCues;return{trace,duration:0,cues:Object.entries(cues).map(([event,time])=>({run:'single',sampleIndex:at(event).index,time,exact:!(id==='integers'&&event==='ready')})),scope:'同一完整C11程序的一次实际GDB运行，两个主题分别显示相关字段；不对应的停点保留完整记录。定位表示本片可见字段已就绪，不是整个机器的逐指令回放。按位运动／权重／推导是数学解释；非法移位和int32有符号溢出被检查拦截，从未执行。'};}
