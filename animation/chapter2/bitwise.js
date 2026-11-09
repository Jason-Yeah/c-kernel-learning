import {mountFilm} from '../shared/player.js';
import {continuity,shot,closeShot,finish,heading,note,pos,register,number,flow,wires} from '../shared/scene-kit.js';
import {integerTrace as t,integerSummary,ready,value,binary,hex,experimentFor} from '../models/integer-bits.js';
import {WORD,word32,setWord,sourceStrip,show,hide,attachIntegerLab} from '../shared/integer-components.js';
const source=ready.fields.bits,mask=ready.fields.mask;
const ops=[['and','&',14,22],['or','|',31.3,37.2],['xor','^',39.3,45.2]];
function retained(){
 const routes=[];
 for(const[event]of ops)WORD.centers.forEach((x,j)=>routes.push([`bw-${event}-a-${j}`,`M${x} 344H${x+46}V506H${x}`,'blue'],[`bw-${event}-b-${j}`,`M${x} 498V506`,'blue'],[`bw-${event}-out-${j}`,`M${x} 536V590`,'cyan']));
 WORD.centers.forEach((x,j)=>routes.push([`bw-not-${j}`,`M${x} 344V450`,'cyan']));
 const oneBits=[...binary(source)].flatMap((b,j)=>b==='1'?[j]:[]);
 for(const dir of ['left','right'])oneBits.forEach(j=>{
  const bit=7-j,target=dir==='left'?bit+2:bit-2,x=WORD.centers[j];
  const dest=target>=8?512:target<0?1310:WORD.centers[7-target];
  const path=target>=8?`M${x} 344H512V486`:`M${x} 344V366H${dest}V450`;
  routes.push([`bw-${dir}-${j}`,path,'cyan']);
 });
 routes.push(['bw-left-zero-0','M1310 438H1210V450','cyan'],['bw-left-zero-1','M1310 438H1118V450','cyan'],['bw-right-zero','M288 390V486','cyan']);
 const s=continuity('bitwise-retained',
  word32('bw-bits',80,220,source,{name:'bits / 源'})+
  word32('bw-mask',80,380,mask,{name:'mask / 源'})+
  word32('bw-work',80,540,0,{name:'按位推导',pending:true,target:true})+
  pos('bw-op',80,496,'&','ibit-operator')+
  `<svg id="bw-gates" class="world-svg" data-anim aria-hidden="true">${WORD.centers.map(x=>`<circle cx="${x}" cy="520" r="12" fill="#000" stroke="#fff"/>`).join('')}</svg>`+
  register('bw-small',80,625,'small / uint8_t',hex(ready.fields.small,8),{width:300})+
  register('bw-promoted',430,625,'~small / int (32位)','尚未计算',{width:360})+
  register('bw-narrow',850,625,'转 uint8_t','尚未转换',{width:350})+
  pos('bw-left-zero-label',1310,400,'补0','ibit-short',{width:100})+
  pos('bw-right-zero-label',300,350,'高位补0','ibit-short',{width:200})+
  register('bw-left-saved',80,625,'r_left / uint32_t',hex(0),{width:520})+
  wires('bw-wires',routes,{packetRadius:4}),96);
 s.root.classList.add('integer-bits-actors');s.$('#bw-work').dataset.role='mathematical-derivation';
 show(s,'bw-bits',.4);show(s,'bw-mask',1.2);show(s,'bw-work',13);show(s,'bw-op',13);show(s,'bw-gates',13);
 for(const[event,operator,start,commit]of ops){
  setWord(s,'bw-work',0,start-.3,{pending:true,name:`推导 r_${event}`});number(s.timeline,s.$('#bw-op'),operator,start-.3);
  const result=value(event,'r_'+event);
  WORD.centers.forEach((_,j)=>{
   const at=start+j*.5;
   flow(s,`bw-${event}-a-${j}`,at,.65);flow(s,`bw-${event}-b-${j}`,at+.18,.45);flow(s,`bw-${event}-out-${j}`,at+.8,.55);
   number(s.timeline,s.$(`#bw-work [data-bit="${7-j}"] .bit-value`),binary(result)[j],at+1.5);
  });setWord(s,'bw-work',result,commit);
 }
 hide(s,'bw-mask',48);hide(s,'bw-op',48);hide(s,'bw-gates',48);
 s.timeline.to(s.$('#bw-work'),{top:400,duration:.9,ease:'power2.inOut'},48.4);
 setWord(s,'bw-work',0,49.5,{pending:true,name:'推导 r_not'});
 WORD.centers.forEach((_,j)=>{flow(s,`bw-not-${j}`,50+j*.35,.65);number(s.timeline,s.$(`#bw-work [data-bit="${7-j}"] .bit-value`),binary(value('not','r_not'))[j],50+j*.35+.8);});
 setWord(s,'bw-work',value('not','r_not'),54);
 show(s,'bw-small',59);show(s,'bw-promoted',60);show(s,'bw-narrow',60.8);
 number(s.timeline,s.$('#bw-promoted .reg-value'),String(value('promotion','promoted_not')),64);
 number(s.timeline,s.$('#bw-narrow .reg-value'),hex(value('promotion','narrow_not'),8),64);
 for(const id of ['bw-small','bw-promoted','bw-narrow'])hide(s,id,68);
 for(const[dir,start,commit]of [['left',70,75.6],['right',78,83.6]]){
  setWord(s,'bw-work',0,start-.7,{pending:true,name:`推导 r_${dir}`});
  oneBits.forEach((j,i)=>flow(s,`bw-${dir}-${j}`,start+i*.55,1));
  setWord(s,'bw-work',value(dir,'r_'+dir),commit);
 }
 show(s,'bw-left-zero-label',73);flow(s,'bw-left-zero-0',73.3,.6);flow(s,'bw-left-zero-1',73.7,.6);hide(s,'bw-left-zero-label',77);
 show(s,'bw-right-zero-label',81);flow(s,'bw-right-zero',81.3,.7);hide(s,'bw-right-zero-label',86);
 show(s,'bw-left-saved',75.7);number(s.timeline,s.$('#bw-left-saved .reg-value'),hex(value('left','r_left')),75.6);
 finish(s.timeline,96);return s;
}
function local(id,duration,eyebrow,title,body,stops,checks,animate){
 const s=shot(id,`<div class="camera">${heading(id+'-head',80,90,eyebrow,title)}${body}</div>`,duration,{stops,checks});s.root.classList.add('integer-bits-scene');show(s,id+'-head',.05);animate?.(s);return closeShot(s,{exit:false});
}
const scenes=[
 {title:'位运算的数值与类型',view:'C / UNSIGNED 32',build:()=>local('bw-program',12,'C BITWISE / FIXED TYPES','先看类型，再看每一位。',
  sourceStrip('bw-program-c',80,555,['uint32_t bits =','uint8_t small ='])+
  note('bw-program-note',80,655,'放大低8位；左边明确折叠高24位。','这是按位权排列，不是内存地址图。源变量、类型与32位边界后面继续保留。',{width:1250}),[8,11],[{at:8,selector:'#bw-bits .ibit-full',text:hex(source)}],s=>{show(s,'bw-program-c',2);show(s,'bw-program-note',6);})},
 {title:'& 掩码，&& 逻辑判断',view:'AND / MASK',build:()=>local('bw-and',18,'AND / KEEP SELECTED BITS','&：掩码为1的位置，才保留原位。',
  sourceStrip('bw-and-c',80,694,['r_and = bits & mask;'])+
  note('bw-and-note',650,690,'位与是6；逻辑&&是1。','逐位揭示是推导，不是CPU逐位store。',{width:680}),[10.2,12,17],[{at:12,selector:'#bw-work .ibit-full',text:hex(value('and','r_and'))}],s=>{show(s,'bw-and-c',1);show(s,'bw-and-note',11);})},
 {title:'| 置位，^ 翻转',view:'OR / XOR',build:()=>local('bw-or-xor',18,'SAME SOURCES / NEW OPERATIONS','|：任一为1；^：两位不同时为1。',
  sourceStrip('bw-or-c',80,694,['r_or = bits | mask;'])+sourceStrip('bw-xor-c',80,694,['r_xor = bits ^ mask;'])+
  note('bw-or-note',650,690,'OR保留共同的1；XOR把共同的1消掉。',`同一输入：OR=${hex(value('or','r_or'))}，XOR=${hex(value('xor','r_xor'))}。^不是乘方。`,{width:680}),[8,16,17],
  [{at:8,selector:'#bw-work .ibit-full',text:hex(value('or','r_or'))},{at:16,selector:'#bw-work .ibit-full',text:hex(value('xor','r_xor'))}],s=>{show(s,'bw-or-c',.6);hide(s,'bw-or-c',9);show(s,'bw-xor-c',9.4);show(s,'bw-or-note',15.5);})},
 {title:'~ 与整型提升',view:'NOT / INTEGER PROMOTIONS',build:()=>local('bw-not-promotion',20,'NOT / PROMOTE BEFORE OPERATING','按位取反前，先看运算的类型。',
  sourceStrip('bw-not-c',80,694,['r_not = ~bits;'])+
  sourceStrip('bw-promotion-c',80,550,['promoted_not = ~small;','narrow_not = (uint8_t)promoted_not;'])+
  pos('bw-promotion-note',80,740,`~small先提升为int(32位)：-183；再转uint8才是49。small<<8实际为int(32位)的${hex(value('promotion','small_shift'))}。`,'ibit-short',{width:1280}),[8,17,19],
  [{at:8,selector:'#bw-work .ibit-full',text:hex(value('not','r_not'))},{at:17,selector:'#bw-promoted .reg-value',text:String(value('promotion','promoted_not'))}],s=>{show(s,'bw-not-c',1.5);hide(s,'bw-not-c',10);show(s,'bw-promotion-c',11);show(s,'bw-promotion-note',16.5);})},
 {title:'<< 与 >>：宽度不变',view:'UNSIGNED SHIFT / FIXED 32',build:()=>local('bw-shift',18,'MOVE BITS / DO NOT SHRINK TO A BYTE','移的是位；运算宽度仍是32位。',
  sourceStrip('bw-left-c',80,550,['r_left = bits << 2;'])+sourceStrip('bw-right-c',80,550,['r_right = bits >> 2;'])+
  note('bw-shift-note',700,640,'左移得到2D8，不是D8。','进入高24位的1仍在32位边界内。无符号右移在高位补0。',{width:650}),[9,17],
  [{at:9,selector:'#bw-work .ibit-full',text:hex(value('left','r_left'))},{at:17,selector:'#bw-work .ibit-full',text:hex(value('right','r_right'))}],s=>{show(s,'bw-left-c',1.1);hide(s,'bw-left-c',9);show(s,'bw-right-c',9.4);show(s,'bw-shift-note',8);})},
 {title:'六种操作与移位边界',view:'TYPE / WIDTH / VALID COUNT',build:()=>local('bw-summary',10,'C11 / NO INVALID OPERATION EXECUTED','位操作先看类型、提升和有效范围。',
  pos('bw-six',80,555,'&　|　^　~　<<　>>','ibit-operator')+
  pos('bw-blocked',730,585,'<strong>count=32：拒绝</strong>本例提升后的左类型为32位。<br>移位次数必须在0…31；未执行非法表达式。','ibit-blocked',{width:630})+
  pos('bw-summary-note',80,740,'位与／位或 ≠ 逻辑与／逻辑或。C11负数右移为实现定义；本片只用无符号移位。','ibit-short',{width:1280}),[7,9],
  [{at:9,selector:'#bw-bits .ibit-full',text:hex(source)}],s=>{show(s,'bw-six',.6);show(s,'bw-blocked',6);show(s,'bw-summary-note',6.5);})}
];
mountFilm({id:'bitwise',chapter:2,title:'C位运算：类型、掩码与移位',example:'../experiments/integer-bits.c',
 model:'C11 / Linux x86-64 · C结果与类型实测 · 位权图不是地址图 · 逐位运动是数学解释，不是CPU时钟',
 provenance:{...t.provenance,samples:integerSummary.samples,events:integerSummary.events,sourceHref:'../experiments/integer-bits.c',jsonHref:'../traces/integer-bits.json',gdbHref:'../traces/integer-bits-gdb.txt'},
 experiment:experimentFor('bitwise'),buildContinuity:retained,scenes});attachIntegerLab();
