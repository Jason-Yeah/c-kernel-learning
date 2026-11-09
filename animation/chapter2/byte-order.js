import {mountFilm} from '../shared/player.js';
import {continuity, shot, closeShot, finish, heading, note, pos, code, register, reveal, vanish, number, flow, wires, COLORS} from '../shared/scene-kit.js';
import {memory} from '../shared/vertical-memory.js';
import {byteTrace as t, byteSummary, ready, at, rows, wordHex, byteHex, sourcePart, experiment, readTimes, encodeTimes} from '../models/byte-order.js';
const show = (s,id,time) => reveal(s.timeline,s.$('#'+id),time,{y:0});
const put = (s,selector,value,time) => number(s.timeline,s.$(selector),value,time);

function retainedObjects() {
  const reads = [0,1,2,3].map(i => [`bo-read-${i}`,`M825 ${635-i*72}H740V410H695`,'blue',byteHex(at('read-'+i).fields.probe_byte)]);
  const encodes = [0,1,2,3].map(i => [`bo-encode-${i}`,`M655 270V315H760V${635-i*72}H825`,'cyan',byteHex(at('encode-'+i).objects.be_encoded.bytes[i])]);
  const routes = [...reads,...encodes,
    ['bo-native-flow','M555 635H575V390H595','blue','读'],
    ['bo-raw-flow','M885 635H875V540H865','blue','读'],
    ['bo-decode-flow','M885 419H875V690H865','cyan','BE']];
  const s = continuity('byte-order-retained',
    register('bo-word',80,225,'word / 同一个 uint32_t',wordHex(ready.fields.word),{width:560})+
    register('bo-address',80,385,'&word / 本次实测虚拟地址',ready.objects.word.address,{width:650})+
    pos('bo-significance',700,360,`<strong>按位权分组 · 不是另一个内存对象</strong><div class="significance-bytes">${byteSummary.big.map((b,i)=>`<div id="bo-significance-${i}" data-anim><b>${byteHex(b)}</b><span>bits ${31-8*i}…${24-8*i}</span></div>`).join('')}</div>`,'significance',{width:620})+
    memory('bo-native',840,330,rows('word'),{width:500,cellHeight:72,title:'word / 小端实测 · 原对象不改',direction:'高 VA ↑ · 低地址在下 · 各对象独立放大'})+
    register('bo-probe',80,350,'probe_byte / 本次已读取的字节',byteHex(ready.fields.probe_byte),{width:600})+
    memory('bo-be',840,330,rows('be_encoded'),{width:500,cellHeight:72,title:'be_encoded / 独立BE编码数组',direction:'高 VA ↑ · 本机显式编码，不是切换CPU端序'})+
    register('bo-native-read',610,330,'本机序读 word',wordHex(ready.fields.native_read),{width:240})+
    register('bo-raw-read',610,480,'直接读 BE 数组',wordHex(ready.fields.raw_read),{width:240})+
    register('bo-decoded',610,630,'按 BE 规则解码',wordHex(ready.fields.decoded_be),{width:240})+
    wires('bo-shared-wires',routes),96);
  s.root.classList.add('byte-order-actors');
  s.$('#bo-address').classList.add('measured-address');
  show(s,'bo-word',0.4); show(s,'bo-address',1);
  s.timeline.to(s.$('#bo-word'),{top:190,duration:0.8,ease:'power2.inOut'},12);
  // Empty right-hand space is used before its future content is revealed.
  s.timeline.to(s.$('#bo-address'),{left:700,duration:0.8,ease:'power2.inOut'},12);
  s.timeline.to(s.$('#bo-address'),{top:190,duration:0.8,ease:'power2.inOut'},12.85);
  show(s,'bo-significance',14.5);
  [0,1,2,3].forEach(i=>show(s,'bo-significance-'+i,15+i*1.3));
  s.timeline.to(s.$('#bo-significance'),{left:80,top:520,duration:1.2,ease:'power2.inOut'},26);
  show(s,'bo-native',27.4); show(s,'bo-probe',27.6);
  reads.forEach(([key],i)=>{
    flow(s,key,29+i*3,1.1);
    put(s,'#bo-probe .reg-value',byteHex(at('read-'+i).fields.probe_byte),readTimes[i]);
  });
  vanish(s.timeline,s.$('#bo-significance'),44,{y:0,duration:0.35});
  vanish(s.timeline,s.$('#bo-probe'),44,{y:0,duration:0.35});
  s.timeline.to(s.$('#bo-native'),{left:80,duration:1.2,ease:'power2.inOut'},44.5);
  show(s,'bo-be',49.2);
  encodes.forEach(([key],i)=>{
    flow(s,key,50+i*2,1);
    put(s,`#bo-be-${i} .array-value`,byteHex(at('encode-'+i).objects.be_encoded.bytes[i]),encodeTimes[i]);
  });
  s.timeline.to(s.$('#bo-native'),{left:40,duration:1.1,ease:'power2.inOut'},62);
  s.timeline.to(s.$('#bo-be'),{left:900,duration:1.1,ease:'power2.inOut'},62);
  show(s,'bo-native-read',63.3);
  flow(s,'bo-native-flow',64,1.1);
  put(s,'#bo-native-read .reg-value',wordHex(at('native-read').fields.native_read),65.18);
  show(s,'bo-raw-read',69.3);
  flow(s,'bo-raw-flow',70,1.1);
  put(s,'#bo-raw-read .reg-value',wordHex(at('raw-read').fields.raw_read),71.18);
  s.timeline.set(s.$('#bo-raw-read .reg-value'),{color:COLORS.red},71.18);
  show(s,'bo-decoded',75.3);
  flow(s,'bo-decode-flow',76,1.1);
  put(s,'#bo-decoded .reg-value',wordHex(at('decoded').fields.decoded_be),77.18);
  for (const id of ['bo-native-read','bo-raw-read','bo-decoded']) vanish(s.timeline,s.$('#'+id),82,{duration:0.35,y:0});
  s.timeline.to(s.$('#bo-native'),{left:180,top:320,duration:1.1,ease:'power2.inOut'},82.45);
  s.timeline.to(s.$('#bo-be'),{left:800,top:320,duration:1.1,ease:'power2.inOut'},82.45);
  finish(s.timeline,96);
  return s;
}
function local(id, duration, eyebrow, title, body, stops, checks, animate) {
  const s = shot(id,`<div class="camera">${heading(id+'-head',80,90,eyebrow,title)}${body}</div>`,duration,{stops,checks});
  s.root.classList.add('byte-order-scene');
  // No full-screen opacity exit: the shared actors remain continuously visible.
  show(s,id+'-head',0.05);
  animate?.(s);
  return closeShot(s,{exit:false});
}
function program() {
  const src = sourcePart(['uint32_t word =','unsigned char be_encoded[4]','const unsigned char *view =','probe_byte = view[read_index];']);
  return local('bo-program',12,'REAL C / ONE VALUE / ONE EXECUTION','同一个数值，怎样占据四个字节？',
    code('bo-program-code',800,235,src.lines,{width:550,title:'byte-order.c / 实际源码节选',lineNumbers:src.numbers})+
    note('bo-program-note',80,575,'这个word与地址，后面一直保留。','切镜只重排仍需使用的对象。完整C、12个实际停点和原样GDB在下方实验台。',{width:1230}),[8,11],
    [{at:8,selector:'#bo-word .reg-value',text:wordHex(ready.fields.word)}],s=>{show(s,'bo-program-code',1.6);show(s,'bo-program-note',6);});
}
function significance() {
  return local('bo-bits',14,'NUMERIC SIGNIFICANCE ≠ MEMORY ORDER','先拆位权，暂时不谈内存地址。',
    note('bo-bits-note',80,590,'左边是高位字节，右边是低位字节。','12对应bits31…24，78对应bits7…0。这是数值的写法，不能直接当作低地址→高地址的内存排列。',{width:1230}),[8,12],[],s=>show(s,'bo-bits-note',8));
}
function native() {
  const src=sourcePart(['probe_byte = view[read_index];']);
  return local('bo-little',18,'NATIVE x86-64 / LOW BYTE AT LOW ADDRESS','小端：低位的78，位于最低地址。',
    code('bo-read-code',80,650,src.lines,{width:650,title:'读已有word，不写word / 读取顺序 +0→+3',lineNumbers:src.numbers}),[4.3,7.3,10.3,13.3,17],
    [{at:16,selector:'#bo-probe .reg-value',text:byteHex(at('read-3').fields.probe_byte)}],s=>show(s,'bo-read-code',1.8));
}
function bigEncoding() {
  const src=sourcePart(['be_encoded[encode_index] =']);
  return local('bo-big',18,'EXPLICIT BIG-ENDIAN ENCODING / SAME HOST','保留原内存，再生成一份大端编码。',
    code('bo-encode-code',700,355,src.lines,{width:650,title:'真实C：高位字节先写到数组低地址',lineNumbers:src.numbers}),[7.2,9.2,11.2,13.2,17],
    [0,1,2,3].map(i=>({at:16,selector:`#bo-be-${i} .array-value`,text:byteHex(at('encode-3').objects.be_encoded.bytes[i])})),s=>{
      show(s,'bo-encode-code',1.8); vanish(s.timeline,s.$('#bo-encode-code'),4,{y:0,duration:0.4});
    });
}
function reconstruct() {
  return local('bo-readback',20,'SAME BYTES / DIFFERENT INTERPRETATION','按什么顺序解释，决定读回什么数。','',[3.4,9.4,15.4,19],
    [{at:18,selector:'#bo-native-read .reg-value',text:wordHex(at('native-read').fields.native_read)},
     {at:18,selector:'#bo-raw-read .reg-value',text:wordHex(at('raw-read').fields.raw_read)},
     {at:18,selector:'#bo-decoded .reg-value',text:wordHex(at('decoded').fields.decoded_be)}]);
}
function summary() {
  return local('bo-summary',14,'BYTE ORDER ≠ BIT REVERSAL','值没变；改变的是字节与地址的对应。',
    note('bo-final-note',80,690,'看低地址端：小端是78，大端编码是12。','每字节的bit不镜像翻转：0x12仍写作0001 0010。并排两列是同一进程的两个不同对象，各自放大。',{width:1240}),[9,13],
    [{at:12,selector:'#bo-word .reg-value',text:wordHex(ready.fields.word)}],s=>show(s,'bo-final-note',3));
}
const p=t.provenance;
mountFilm({id:'byte-order',chapter:2,title:'字节序：同一个数值的内存排列',
  model:'Linux x86-64 小端实测 · BE为本机显式编码 · 各对象独立放大 · 箭头与教学时间为解释设计',
  example:'../experiments/byte-order.c',
  provenance:{...p,samples:byteSummary.samples,events:byteSummary.events,sourceHref:'../experiments/byte-order.c',jsonHref:'../traces/byte-order.json',gdbHref:'../traces/byte-order-gdb.txt'},
  experiment,buildContinuity:retainedObjects,
  scenes:[
    {title:'一个word，保留到最后',view:'C / SAME OBJECT',build:program},
    {title:'拆位权，不是排地址',view:'SIGNIFICANCE',build:significance},
    {title:'小端：读原内存',view:'NATIVE LE / READ',build:native},
    {title:'显式生成BE编码',view:'EXPLICIT BE / WRITE',build:bigEncoding},
    {title:'读回与正确解释',view:'MEMCPY / BE DECODE',build:reconstruct},
    {title:'保留两列，总结对照',view:'SAME VALUE / TWO ENCODINGS',build:summary}
  ]});
document.querySelector('.experiment-links').insertAdjacentHTML('beforeend','<a href="../traces/byte-order/program.elf">本次实验ELF ↗</a><a href="./byte-order.md">本片说明 ↗</a>');
const observations=document.createElement('p');observations.id='byte-order-observation';observations.className='byte-order-observation';
document.querySelector('#experiment-observation').after(observations);
function updateObservation(){
  const s=t.runs.single.samples[Number(document.querySelector('#experiment-stop').value)];
  observations.textContent=`word @ ${s.objects.word.address} = ${wordHex(s.fields.word)}；内存按地址升序：${s.objects.word.bytes.map(byteHex).join(' ')}。be_encoded @ ${s.objects.be_encoded.address}：${s.objects.be_encoded.bytes.map(byteHex).join(' ')}；native_read=${wordHex(s.fields.native_read)}，raw_read=${wordHex(s.fields.raw_read)}，decoded_be=${wordHex(s.fields.decoded_be)}。`;
}
document.querySelector('#experiment-stop').addEventListener('change',updateObservation);updateObservation();
