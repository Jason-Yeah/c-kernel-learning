import trace from '../traces/control-flow.json' with {type:'json'};
import {validateControlFlow,procFields} from './validate-control-flow.js';
export const controlTrace=trace;
export const controlSummary=validateControlFlow(trace);
export function at(run,event){const s=trace.runs[run]?.samples.find(s=>s.event===event);if(!s)throw Error('Missing control-flow observation: '+run+'/'+event);return s;}
export const ready=at('inspect','ready'),entered=at('inspect','function-entry'),returned=at('inspect','function-return'),stored=at('inspect','sum-stored'),before=at('inspect','syscall-before'),after=at('inspect','syscall-after'),pidStored=at('inspect','pid-stored');
export const faultReady=at('fault','fault-ready'),faultBefore=at('fault','fault-before'),faultSignal=at('fault','fault-signal');
export const blocking=trace.os.blocking,blockReady=blocking.records.find(r=>r.event==='blocking-ready'),helper=blocking.records.find(r=>r.event==='helper-write'),blockResult=blocking.records.find(r=>r.event==='blocking-result');
export const hx=n=>'0x'+BigInt(n).toString(16).toUpperCase();
export const dec=n=>BigInt(n).toString();
export const byte=n=>n.toString(16).toUpperCase().padStart(2,'0');
export function sourcePart(terms){const lines=trace.provenance.source.content.split('\n');const numbers=terms.map(term=>{const matches=lines.flatMap((line,i)=>line.includes(term)?[i+1]:[]);if(matches.length!==1)throw Error('Ambiguous control-flow C source: '+term);return matches[0];});return{lines:numbers.map(i=>lines[i-1].replace(/\s*\/\*.*?\*\//g,'').trim()),numbers};}
const points={inspect:{ready:8,'function-entry':16,'function-return':22,'sum-stored':24,'syscall-before':32,'syscall-after':51,'pid-stored':55},fault:{'fault-ready':80,'fault-before':85,'fault-signal':90}};
export const experiment={trace,duration:0,cues:Object.entries(points).flatMap(([run,times])=>Object.entries(times).map(([event,time])=>({run,sampleIndex:at(run,event).index,time,exact:true}))),scope:'两次独立GDB运行：inspect观察用户CALL与SYSCALL边界；fault观察自有保护页的真实SIGSEGV。另有独立、非GDB的blocking运行与/proc／rusage记录，见下方OS实验台。精确定位指本片可见用户状态已匹配，不是实时内核录屏。进入内核、硬件IRQ及调度箭头均为明确机制解释。'};
export const blockParentStatus=procFields(blocking.beforeRelease.files.find(f=>f.path===`/proc/${blockReady.pid}/status`).content);
