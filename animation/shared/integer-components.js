import {pos,escape,number,reveal,vanish} from './scene-kit.js';
import {binary,hex,integerTrace as trace,sourcePart} from '../models/integer-bits.js';
export const WORD={x:80,lowX:524,cell:84,gap:8,centers:Array.from({length:8},(_,j)=>566+j*92)};
export function word32(id,x,y,value,{name='bits',type='uint32_t · 32位',pending=false,target=false}={}){
 const bits=binary(value,32).slice(-8);
 return pos(id,x,y,`<strong class="ibit-name">${escape(name)}</strong><span class="ibit-type">${escape(type)}</span><b class="ibit-full">${pending?'推导中':hex(value)}</b><span class="ibit-fold-index">31…8 · 高24位折叠</span><b class="ibit-fold-value">${pending?'·':'0x'+(BigInt.asUintN(32,BigInt(value))>>8n).toString(16).toUpperCase().padStart(6,'0')}</b>${[...bits].map((b,j)=>`<span class="ibit-index" style="left:${444+j*92}px">${7-j}</span><span class="ibit-cell" data-bit="${7-j}" style="left:${444+j*92}px"><b class="bit-value">${pending?'·':b}</b></span>`).join('')}`,
 `ibit-word ${target?'ibit-target':''}`,{width:1280}).replace('class="positioned','data-bit-width="32" class="positioned');
}
export function setWord(s,id,value,time,{pending=false,name,type}={}){
 const root=s.$('#'+id);if(name)number(s.timeline,root.querySelector('.ibit-name'),name,time);if(type)number(s.timeline,root.querySelector('.ibit-type'),type,time);
 number(s.timeline,root.querySelector('.ibit-full'),pending?'推导中':hex(value),time);
 number(s.timeline,root.querySelector('.ibit-fold-value'),pending?'·':'0x'+(BigInt.asUintN(32,BigInt(value))>>8n).toString(16).toUpperCase().padStart(6,'0'),time);
 [...binary(value,32).slice(-8)].forEach((bit,j)=>number(s.timeline,root.querySelector(`[data-bit="${7-j}"] .bit-value`),pending?'·':bit,time));
}
export function bitRibbon(id,x,y,value,{name='编码',width=8,cell=72,gap=12,pending=false,type=''}={}){
 const total=width*cell+(width-1)*gap;
 return pos(id,x,y,`<strong class="ibit-ribbon-name">${escape(name)}</strong><code class="ibit-ribbon-hex">${pending?'待推导':hex(value,width)}</code>${type?`<span class="ibit-ribbon-type">${escape(type)}</span>`:''}${[...binary(value,width)].map((b,j)=>`<span class="ibit-ribbon-index" style="left:${j*(cell+gap)}px;width:${cell}px">${width-1-j}</span><span class="ibit-ribbon-cell" data-bit="${width-1-j}" style="left:${j*(cell+gap)}px;width:${cell}px"><b class="bit-value">${pending?'·':b}</b></span>`).join('')}`,'ibit-ribbon',{width:total}).replace('class="positioned',`data-bit-width="${width}" class="positioned`);
}
export function setRibbon(s,id,value,time){const r=s.$('#'+id),width=Number(r.dataset.bitWidth);number(s.timeline,r.querySelector('.ibit-ribbon-hex'),hex(value,width),time);[...binary(value,width)].forEach((b,j)=>number(s.timeline,r.querySelector(`[data-bit="${width-1-j}"] .bit-value`),b,time));}
export function sourceStrip(id,x,y,terms,width=1200){const src=sourcePart(terms);return pos(id,x,y,src.lines.map((line,j)=>`<div class="ibit-codeline" data-source-line="${src.numbers[j]}"><i>${src.numbers[j]}</i><code>${escape(line)}</code></div>`).join(''),'ibit-source',{width});}
export const show=(s,id,time)=>reveal(s.timeline,s.$('#'+id),time,{y:0});
export const hide=(s,id,time,duration=.35)=>vanish(s.timeline,s.$('#'+id),time,{y:0,duration});
export function attachIntegerLab(){
 document.querySelector('.experiment-links').insertAdjacentHTML('beforeend','<a href="../traces/integer-bits/program.elf">本次ELF ↗</a><a href="../traces/integer-bits/sanitizer.txt">实际UBSan报告 ↗</a><a href="./guide.html">整数与C位运算讲义 ↗</a>');
 const box=document.createElement('details');box.className='integer-observations';box.open=true;
 box.innerHTML='<summary>所选停点：实际C类型、值与内存字节</summary><p>字节按地址升序列出；与动画按位权从高到低的图不同。signed_result保持初始化的0表示没有赋值，不是溢出算得0。</p><div class="integer-table-scroll"><table><thead><tr><th>对象</th><th>GDB实际类型／位宽</th><th>有符号性</th><th>数值</th><th>内存字节</th><th>实测VA</th></tr></thead><tbody id="integer-observation-rows"></tbody></table></div><h3>实际GDB表达式类型（整型提升）</h3><pre id="integer-expression-types"></pre>';
 document.querySelector('#experiment-workbench').after(box);
 document.querySelector('#integer-expression-types').textContent=trace.types.map(t=>`(gdb) ${t.evidence.command}\n${t.evidence.output}\n实际宽度=${t.width}，${t.signed?'signed':'unsigned'}`).join('\n\n');
 function update(){const s=trace.runs.single.samples[Number(document.querySelector('#experiment-stop').value)];document.querySelector('#integer-observation-rows').innerHTML=Object.entries(s.objects).map(([name,o])=>`<tr data-field="${name}"><th>${name}</th><td>${escape(o.type)} / ${o.width}</td><td>${o.signed?'signed':'unsigned'}</td><td>${s.fields[name]}</td><td>${o.bytes.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ')}</td><td>${o.address}</td></tr>`).join('');}
 document.querySelector('#experiment-stop').addEventListener('change',update);update();
}
