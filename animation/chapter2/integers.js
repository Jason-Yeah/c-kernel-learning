import {mountFilm} from '../shared/player.js';
import {continuity,shot,closeShot,finish,heading,note,pos,register,number,flow,wires} from '../shared/scene-kit.js';
import {integerTrace as t,integerSummary,ready,value,binary,hex,experimentFor} from '../models/integer-bits.js';
import {bitRibbon,setRibbon,sourceStrip,show,hide,attachIntegerLab} from '../shared/integer-components.js';
const d6=ready.fields.u8;
function compact(s,id,x,y,width,time){
 const root=s.$('#'+id);
 s.timeline.to(root.querySelectorAll('.ibit-ribbon-cell,.ibit-ribbon-index'),{autoAlpha:0,duration:.3},time);
 s.timeline.to(root.querySelectorAll('.ibit-ribbon-name,.ibit-ribbon-hex'),{top:0,duration:.9,ease:'power2.inOut'},time+.3);
 s.timeline.to(root,{left:x,top:y,width,duration:.9,ease:'power2.inOut'},time+.3);
 s.timeline.set(root,{attr:{'data-folded':'true'}},time+1.2);
}
function retained(){
 const routes=[];
 const ones=[...binary(d6)].flatMap((b,j)=>b==='1'?[j]:[]);
 ones.forEach((j,i)=>{const x=416+j*84;routes.push([`int-weight-u-${i}`,`M${x} 390H60V580H80`,'blue'],[`int-weight-s-${i}`,`M${x} 390H1380V580H1340`,'blue']);});
 for(let j=0;j<8;j++){
  routes.push([`int-invert-${j}`,`M1330 450V510H${536+j*84}V588`,'cyan']);
  const low=922+j*56,high=474+j*56;
  routes.push([`int-zero-low-${j}`,`M400 280V400H${low}V382`,'blue'],[`int-zero-high-${j}`,`M400 385H${high}V382`,'cyan']);
  routes.push([`int-sign-low-${j}`,`M400 500V572H${low}`,'blue'],[`int-sign-high-${j}`,`M65 681V572H${high}`,'cyan']);
  const from=160+(24+j)*34+14,to=696+j*84;
  routes.push([`int-trunc-${j}`,`M${from} 395V450H${to}V558`,'cyan']);
 }
 routes.push(['int-neg-carry','M1200 578H1040','cyan'],['int-carry','M1228 385V415H140','cyan'],['int-wide-flow','M620 390H780','cyan']);
 const s=continuity('integers-retained',
  bitRibbon('int-pattern',380,255,d6,{name:'同一编码 / 8位'})+
  register('int-u8',80,450,'u8 / uint8_t',String(ready.fields.u8),{width:500})+
  register('int-s8',860,450,'s8 / int8_t',String(ready.fields.s8),{width:500})+
  register('int-magnitude',760,390,'magnitude / uint8_t',String(ready.fields.magnitude),{width:550})+
  bitRibbon('int-neg-work',500,530,ready.fields.magnitude,{name:'编码推导 / 8位'})+
  bitRibbon('int-zero',450,250,0,{name:'zero_extended / uint16_t',width:16,cell:48,gap:8,pending:true})+
  bitRibbon('int-sign',450,440,0,{name:'sign_extended / int16_t',width:16,cell:48,gap:8,pending:true})+
  bitRibbon('int-wide',160,260,ready.fields.wide_unsigned,{name:'wide_unsigned / uint32_t',width:32,cell:28,gap:6})+
  bitRibbon('int-truncated',660,500,0,{name:'truncated / uint8_t',pending:true})+
  bitRibbon('int-wrap-source',160,240,ready.fields.wrap_lhs,{name:'wrap_lhs / uint32_t',width:32,cell:28,gap:6})+
  bitRibbon('int-wrapped',160,470,0,{name:'wrapped / uint32_t',width:32,cell:28,gap:6,pending:true})+
  register('int-signed-lhs',80,330,'signed_lhs / int32_t',String(ready.fields.signed_lhs),{width:520})+
  register('int-wide-sum',800,330,'wide_sum / int64_t','尚未计算',{width:560})+
  register('int-promoted',80,565,'small_max+1 / int','尚未计算',{width:620})+
  pos('int-rejected',800,550,'<strong>int32结果：拒绝产生</strong>2147483648超出INT32_MAX。<br>未执行有符号溢出的int32相加。','ibit-blocked',{width:560})+
  wires('int-shared-wires',routes,{packetRadius:4}),128);
 s.root.classList.add('integer-bits-actors');
 for(const id of ['int-neg-work','int-truncated','int-wrapped'])s.$('#'+id).classList.add('ibit-incoming','ibit-output');
 for(const id of ['int-zero','int-sign'])s.$('#'+id).classList.add('ibit-output');
 s.$('#int-neg-work').dataset.role='mathematical-derivation';
 show(s,'int-pattern',.5);show(s,'int-u8',1);show(s,'int-s8',1.6);
 ones.forEach((_,i)=>{flow(s,'int-weight-u-'+i,14+i*.65,.9);flow(s,'int-weight-s-'+i,20+i*.65,.9);});
 s.timeline.to(s.$('#int-pattern'),{left:740,top:220,duration:1.2,ease:'power2.inOut'},30);
 s.timeline.to(s.$('#int-u8'),{top:220,duration:1,ease:'power2.inOut'},31.3);
 s.timeline.to(s.$('#int-s8'),{left:80,top:360,duration:1,ease:'power2.inOut'},31.3);
 show(s,'int-magnitude',33);show(s,'int-neg-work',33.7);
 for(let j=0;j<8;j++)flow(s,'int-invert-'+j,34.2+j*.13,.65);
 setRibbon(s,'int-neg-work',value('invert8','inverted8'),36);
 number(s.timeline,s.$('#int-neg-work .ibit-ribbon-hex'),'进位推导',38);
 flow(s,'int-neg-carry',38,1.6);
 number(s.timeline,s.$('#int-neg-work [data-bit="0"] .bit-value'),'0',39);
 number(s.timeline,s.$('#int-neg-work [data-bit="1"] .bit-value'),'1',40);
 setRibbon(s,'int-neg-work',value('negate8','negated8'),41);
 hide(s,'int-magnitude',46);hide(s,'int-neg-work',46);
 s.timeline.to(s.$('#int-pattern'),{top:585,duration:1,ease:'power2.inOut'},46);
 s.timeline.to(s.$('#int-pattern'),{left:80,duration:.9,ease:'power2.inOut'},47.05);
 s.timeline.to(s.$('#int-u8'),{width:300,duration:.9,ease:'power2.inOut'},46);
 s.timeline.to(s.$('#int-s8'),{top:440,width:300,duration:.9,ease:'power2.inOut'},46);
 show(s,'int-zero',48);show(s,'int-sign',48.8);
 for(let j=0;j<8;j++){
  flow(s,'int-zero-low-'+j,49.5+j*.18,.8);flow(s,'int-zero-high-'+j,51.7+j*.13,.55);
  flow(s,'int-sign-low-'+j,53.5+j*.18,.8);flow(s,'int-sign-high-'+j,55.2+j*.13,.55);
 }
 setRibbon(s,'int-zero',value('extend','zero_extended'),57.5);setRibbon(s,'int-sign',value('extend','sign_extended'),57.5);
 for(const id of ['int-pattern','int-u8','int-s8','int-zero','int-sign'])hide(s,id,64);
 show(s,'int-wide',65);show(s,'int-truncated',66);
 for(let j=0;j<8;j++)flow(s,'int-trunc-'+j,68+j*.45,1.1);
 setRibbon(s,'int-truncated',value('truncate','truncated'),74);
 compact(s,'int-wide',80,720,560,80);compact(s,'int-truncated',800,720,560,80);
 show(s,'int-wrap-source',82);show(s,'int-wrapped',82.6);flow(s,'int-carry',86,3.3);
 setRibbon(s,'int-wrapped',value('unsigned-wrap','wrapped'),92);
 compact(s,'int-wrap-source',80,210,560,98);compact(s,'int-wrapped',800,210,560,98);
 show(s,'int-signed-lhs',100.5);show(s,'int-wide-sum',101.1);flow(s,'int-wide-flow',104,1.2);
 number(s.timeline,s.$('#int-wide-sum .reg-value'),String(value('signed-guard','wide_sum')),108);
 show(s,'int-rejected',108.3);show(s,'int-promoted',110.4);
 number(s.timeline,s.$('#int-promoted .reg-value'),String(value('promoted-add','promoted_add')),113);
 finish(s.timeline,128);return s;
}
function local(id,duration,eyebrow,title,body,stops,checks,animate){const s=shot(id,`<div class="camera">${heading(id+'-head',80,90,eyebrow,title)}${body}</div>`,duration,{stops,checks});s.root.classList.add('integer-bits-scene');show(s,id+'-head',.05);animate?.(s);return closeShot(s,{exit:false});}
const scenes=[
 {title:'同字节，两个类型',view:'TWO OBJECTS / SAME ENCODING',build:()=>local('int-program',12,'C11 / REAL OBJECT BYTES','同一串D6，为什么能表示两个数？',
  sourceStrip('int-program-c',80,620,['uint8_t u8 =','int8_t s8 ='])+
  note('int-program-note',80,710,'这是两个独立C对象；GDB读到的字节都为D6。','',{width:1280}),[8,11],
  [{at:8,selector:'#int-u8 .reg-value',text:String(ready.fields.u8)},{at:8,selector:'#int-s8 .reg-value',text:String(ready.fields.s8)}],s=>{show(s,'int-program-c',3);show(s,'int-program-note',7);})},
 {title:'补码的负权重',view:'UNSIGNED / TWO’S COMPLEMENT',build:()=>{
  const weights=[128,64,32,16,8,4,2,1];
  return local('int-weights',18,'SAME BITS / DIFFERENT WEIGHTS','补码最高位的权是-128，不是负号。',
   pos('int-weight-label',80,400,'无符号位权','ibit-strong',{width:250})+
   weights.map((w,j)=>pos('int-weight-'+j,380+j*84,400,String(w),'ibit-short',{width:72})).join('')+
   pos('int-u-formula',80,590,'128 + 64 + 16 + 4 + 2 = 214','ibit-formula',{width:600})+
   pos('int-s-formula',860,590,'-128 + 64 + 16 + 4 + 2 = -42','ibit-formula',{width:550})+
   note('int-ranges',80,680,'8位范围：unsigned 0…255；补码 -128…127。','',{width:1280}),[6.2,13,17],[],s=>{
    show(s,'int-weight-label',1);for(let j=0;j<8;j++)show(s,'int-weight-'+j,1.2+j*.1);
    show(s,'int-u-formula',5.8);number(s.timeline,s.$('#int-weight-0'),'-128',7);number(s.timeline,s.$('#int-weight-label'),'补码位权',7);
    show(s,'int-s-formula',12.2);show(s,'int-ranges',14);
   });}},
 {title:'从+42求-42的编码',view:'INVERT LOW 8 / ADD ONE',build:()=>local('int-negate',16,'EXPLICIT UNSIGNED INTERMEDIATES','低8位取反，再加1，得到D6。',
  sourceStrip('int-invert-c',80,470,['inverted8 = (uint8_t)'])+
  sourceStrip('int-negate-c',80,740,['negated8 = (uint8_t)'])+
  pos('int-neg-plus-one',1210,550,'+1','ibit-result',{width:100}),[7,12,15],
  [{at:7,selector:'#int-neg-work .ibit-ribbon-hex',text:hex(value('invert8','inverted8'),8)},{at:12,selector:'#int-neg-work .ibit-ribbon-hex',text:hex(value('negate8','negated8'),8)}],s=>{show(s,'int-invert-c',3.5);show(s,'int-neg-plus-one',7.5);show(s,'int-negate-c',11.3);})},
 {title:'补0与符号扩展',view:'NEW 16-BIT TARGETS / SAME VALUES',build:()=>local('int-extend',18,'ZERO EXTEND / SIGN EXTEND','扩宽时，保留的是数值。',
  sourceStrip('int-extend-c',800,710,['zero_extended = (uint16_t)u8;','sign_extended = (int16_t)s8;'])+
  note('int-extend-note',800,630,'补0，或复制符号位。','214仍是214；-42仍是-42。8位源仍保留。',{width:560}),[12,15,17],
  [{at:13,selector:'#int-zero .ibit-ribbon-hex',text:hex(value('extend','zero_extended'),16)},{at:13,selector:'#int-sign .ibit-ribbon-hex',text:hex(value('extend','sign_extended'),16)}],s=>{show(s,'int-extend-c',12);show(s,'int-extend-note',12.8);})},
 {title:'转unsigned窄类型：保留低位',view:'NEW CASE / DEFINED UNSIGNED CONVERSION',build:()=>local('int-truncate',16,'UINT32 → UINT8 / MODULO 256','转成uint8_t：保留低8位。',
  sourceStrip('int-truncate-c',80,440,['truncated = (uint8_t)wide_unsigned;'])+
  note('int-truncate-note',80,540,'只在目标不保留高位。','源0x1234仍在；目标得到0x34。超范围转signed窄类型不能直接套此规则。',{width:500}),[10.2,13,15],
  [{at:12,selector:'#int-truncated .ibit-ribbon-hex',text:hex(value('truncate','truncated'),8)},{at:12,selector:'#int-wide .ibit-ribbon-hex',text:hex(ready.fields.wide_unsigned)}],s=>{show(s,'int-truncate-c',2);show(s,'int-truncate-note',10.4);})},
 {title:'unsigned模加，不是异常',view:'FIXED 32 / DEFINED WRAP',build:()=>local('int-unsigned',18,'UINT32_MAX + 1u / MODULO 2^32','unsigned：结果按2的位宽次幂取模。',
  sourceStrip('int-wrap-c',80,380,['wrapped = wrap_lhs +'])+
  pos('int-plus-one',1300,400,'+1u','ibit-result',{width:100})+
  pos('int-carry-out',40,440,'边界外进位','ibit-short',{width:150})+
  pos('int-u8-promotion',80,675,`u8_max+1 先提升为int，得到${value('unsigned-wrap','promoted_256')}；再转uint8_t才得${value('unsigned-wrap','wrapped8')}。`,'ibit-short',{width:1280}),[12.2,15,17],
  [{at:14,selector:'#int-wrapped .ibit-ribbon-hex',text:hex(value('unsigned-wrap','wrapped'))}],s=>{show(s,'int-wrap-c',2.8);show(s,'int-plus-one',5.5);show(s,'int-carry-out',9.5);show(s,'int-u8-promotion',13);})},
 {title:'signed溢出被安全拦截',view:'INT64 CHECK / DO NOT EXECUTE UB',build:()=>local('int-signed',18,'C11 SIGNED OVERFLOW ≠ DEFINED WRAP','signed：超范围，不承诺回绕结果。',
  sourceStrip('int-guard-c',80,465,['wide_sum = (int64_t)','if (wide_sum >='])+
  pos('int-widen-first',645,350,'int64 +1','ibit-short',{width:150})+
  pos('int-promotion-note',80,680,`int8_t的127+1先提升为int，实际128；本机-INT8_MIN也得到${value('promoted-add','promoted_negmin')}。`,'ibit-short',{width:1280}),[11,16,17],
  [{at:11,selector:'#int-wide-sum .reg-value',text:String(value('signed-guard','wide_sum'))},{at:16,selector:'#int-promoted .reg-value',text:String(value('promoted-add','promoted_add'))}],s=>{show(s,'int-guard-c',3.3);show(s,'int-widen-first',5);show(s,'int-promotion-note',15.5);})},
 {title:'位串＋类型＋规则',view:'KEEP RESULTS / NEXT: FLOATING POINT',build:()=>local('int-summary',12,'REPRESENTATION + TYPE + OPERATION WIDTH','位串相同，不代表运算规则相同。',
  pos('int-final-rule',80,460,'位串 + 类型 + 运算宽度 + 转换规则','ibit-result',{width:1280})+
  pos('int-final-note',80,510,'unsigned模运算 ≠ signed溢出；小整型先提升，转窄类型再判断转换规则。','ibit-short',{width:1280}),[8,11],
  [{at:10,selector:'#int-truncated .ibit-ribbon-hex',text:hex(value('truncate','truncated'),8)},{at:10,selector:'#int-wrapped .ibit-ribbon-hex',text:hex(value('unsigned-wrap','wrapped'))}],s=>{show(s,'int-final-rule',.7);show(s,'int-final-note',2);})}
];
mountFilm({id:'integers',chapter:2,title:'补码、扩展、截断与溢出',example:'../experiments/integer-bits.c',
 model:'C11 / Linux x86-64 · 值／类型／对象字节实测 · 位权不是地址 · 推导不是CPU时钟 · 未执行signed溢出',
 provenance:{...t.provenance,samples:integerSummary.samples,events:integerSummary.events,sourceHref:'../experiments/integer-bits.c',jsonHref:'../traces/integer-bits.json',gdbHref:'../traces/integer-bits-gdb.txt'},
 experiment:experimentFor('integers'),buildContinuity:retained,scenes});attachIntegerLab();
