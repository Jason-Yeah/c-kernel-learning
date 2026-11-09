import {mountFilm} from '../shared/player.js';
import {continuity,shot,closeShot,finish,pos,heading,register,chip,number,reveal,vanish,wires,flow,COLORS,escape} from '../shared/scene-kit.js';
import {memory} from '../shared/vertical-memory.js';
import {controlTrace as t,controlSummary,ready,entered,returned,stored,before,after,pidStored,faultReady,faultBefore,faultSignal,blockReady as br,helper,blockResult as result,sourcePart,hx,dec,byte,experiment} from '../models/control-flow.js';
import {attachControlLab} from '../shared/control-flow-lab.js';
const show=(s,id,at)=>reveal(s.timeline,s.$('#'+id),at,{y:0});
const hide=(s,id,at)=>vanish(s.timeline,s.$('#'+id),at,{y:0,duration:.35});
const text=(s,selector,value,at)=>number(s.timeline,s.$(selector),value,at);
function source(id,x,y,terms,width=880){const part=sourcePart(terms);return pos(id,x,y,part.lines.map((line,j)=>`<div data-source-line="${part.numbers[j]}"><i>${part.numbers[j]}</i><code>${escape(line)}</code></div>`).join(''),'ec-source',{width});}
function snapshot(s,sample,time){text(s,'#ec-rip .reg-value',sample.registers.rip,time);text(s,'#ec-rax .reg-value',dec(sample.registers.rax),time);}
function moveToken(s,path,at,duration){s.timeline.to(s.$('#ec-execution'),{motionPath:{path},duration,ease:'power1.inOut'},at);}
function retained(){
 const s=continuity('control-flow-retained',
  chip('ec-cpu',80,260,{width:250,label:'CPU',subtitle:'机制框架'})+
  pos('ec-inspect-case',450,195,`inspect独立运行 / PID ${ready.pid}`,'ec-case',{width:880})+
  pos('ec-user',450,230,'<strong>用户态 / 用户代码</strong>','ec-mode',{width:880})+
  pos('ec-kernel',450,450,'<strong>内核态 / 入口与返回是机制解释</strong>','ec-mode ec-kernel',{width:880})+
  source('ec-main-code',480,285,['observed_sum = local_add(input_a, input_b);','observed_pid = raw_getpid();'],550)+
  pos('ec-callee',1050,280,`local_add<span>a=${entered.fields.input_a}，b=${entered.fields.input_b}</span>`,'ec-node',{width:260})+
  pos('ec-wrapper',1050,280,'raw_getpid()<span>用户态包装函数</span>','ec-node',{width:260})+
  pos('ec-kernel-work',700,510,'入口保存／处理／返回','ec-strong',{width:580})+
  pos('ec-opcode',1050,245,'0F 05 / SYSCALL','ec-short',{width:275})+
  register('ec-rip',80,635,'%rip / 用户态边界快照',ready.registers.rip,{width:600})+
  register('ec-rax',760,635,'%rax / 用户态边界快照',dec(ready.registers.rax),{width:500})+
  register('ec-sum',80,540,'inspect / sum',String(ready.fields.observed_sum),{width:260})+
  pos('ec-block-case',450,210,`blocking独立运行 / 仅允许逻辑CPU ${br.cpu}`,'ec-case',{width:890})+
  pos('ec-parent',450,255,`父进程<b>PID ${br.pid}</b><span class="ec-process-state">准备read</span>`,'ec-process',{width:390})+
  pos('ec-helper',950,255,`helper<b>PID ${helper.pid}</b><span class="ec-process-state">等待控制字符!</span>`,'ec-process',{width:390})+
  pos('ec-receiver',450,435,`<strong>父received / 1B</strong><small>@ ${hx(br.buffer)}</small><b>${byte(br.beforeByte)}</b>`,'ec-copy',{width:390})+
  pos('ec-sender',950,435,`<strong>helper sender_byte / 1B</strong><small>读取只复制，源不消失</small><b>${byte(helper.byte)} ('K')</b>`,'ec-copy ec-source-copy',{width:390})+
  pos('ec-pipe-title',870,430,'PIPE','ec-short',{width:80})+
  `<svg id="ec-pipe" class="world-svg" data-anim aria-hidden="true"><path d="M875 462H915V548H875Z M875 478H915 M875 532H915" fill="#000" stroke="#fff"/></svg>`+
  pos('ec-block-meaning',450,570,'接收对象固定1B；管道缓冲容量不按此图比例。','ec-short',{width:890})+
  pos('ec-block-owner',450,600,'父阻塞；CPU可以调度其他任务。','ec-short',{width:890})+
  pos('ec-os-result',450,650,'尚未返回read','ec-short',{width:890})+
  pos('ec-fault-case',950,210,`fault独立运行 / PID ${faultReady.pid}`,'ec-case',{width:390})+
  memory('ec-guard',950,350,[{key:'page',label:`guard_page / ${faultReady.fields.page_size}B`,address:faultReady.guardAddress,value:'RW',role:'target-row'}],{width:390,cellHeight:112,title:'自有连续虚拟页 / 权限',direction:'高 VA ↑ · 页容量固定 · 非物理页号'})+
  register('ec-fault-rip',450,580,'%rip / fault用户态观察点',faultReady.registers.rip,{width:890})+
  register('ec-signal',450,475,'fault运行 / 信号','尚未发生',{width:450})+
  pos('ec-irq-device',1120,480,'<strong>外部设备事件</strong><span>机制示意，未采IRQ</span>','ec-irq',{width:230})+
  pos('ec-irq-handler',450,480,'中断处理，可返回原进程','ec-strong',{width:420})+
  `<svg class="world-svg" id="ec-execution-svg" aria-hidden="true"><g id="ec-execution" class="moving-packet" data-anim><circle r="8" fill="${COLORS.cyan}"/></g></svg>`+
  wires('ec-wires',[
   ['ec-call','M800 390H1000','blue'],['ec-return','M1000 402H800','cyan'],
   ['ec-store-sum','M745 690H700V760H350V590','cyan'],
   ['ec-enter','M1000 390H1360V585H950','cyan'],['ec-leave','M950 585H420V390H800','cyan'],
   ['ec-schedule-helper','M855 400H935','cyan'],['ec-schedule-parent','M935 412H855','cyan'],
   ['ec-copy','M935 495H855','cyan'],
   ['ec-fault-request','M890 430H935','red'],['ec-fault-enter','M935 430V565H420V530H360','red'],
   ['ec-irq','M1100 530H900V565H420V530H360','cyan'],['ec-irq-return','M360 530H410V445H700','cyan']
  ],{packetRadius:6}),124);
 s.root.classList.add('control-flow-actors');s.$('#ec-receiver').dataset.capacity=String(br.capacity);s.$('#ec-sender').dataset.capacity='1';
 for(const id of ['ec-rip','ec-rax','ec-sum'])s.$('#'+id).dataset.run='inspect';
 for(const id of ['ec-parent','ec-helper','ec-receiver','ec-os-result'])s.$('#'+id).dataset.run='blocking';
 for(const id of ['ec-guard','ec-signal','ec-fault-rip'])s.$('#'+id).dataset.run='fault';
 show(s,'ec-cpu',.4);show(s,'ec-inspect-case',.7);show(s,'ec-user',1);show(s,'ec-main-code',2);
 show(s,'ec-rip',3);show(s,'ec-rax',3.4);show(s,'ec-sum',4);
 s.timeline.set(s.$('#ec-execution'),{x:800,y:390,autoAlpha:1},3);
 show(s,'ec-callee',13.2);flow(s,'ec-call',14.3,1.6);moveToken(s,'M800 390H1000',14.3,1.6);snapshot(s,entered,16);
 flow(s,'ec-return',20.4,1.5);moveToken(s,'M1000 390V402H800V390',20.4,1.5);snapshot(s,returned,22);
 // The actual C store commits as a word. This path explains its destination.
 flow(s,'ec-store-sum',22.3,1.5);text(s,'#ec-sum .reg-value',String(stored.fields.observed_sum),24);snapshot(s,stored,24);
 hide(s,'ec-callee',26);show(s,'ec-wrapper',27.2);show(s,'ec-opcode',30);snapshot(s,before,32);
 show(s,'ec-kernel',34);show(s,'ec-kernel-work',35);flow(s,'ec-enter',36,3.4);moveToken(s,'M800 390H1360V585H950',36,3.4);
 flow(s,'ec-leave',47.5,3.1);moveToken(s,'M950 585H420V390H800',47.5,3.1);snapshot(s,after,51);hide(s,'ec-kernel-work',50.7);snapshot(s,pidStored,55);
 hide(s,'ec-rip',55.8);
 s.timeline.to(s.$('#ec-sum'),{top:675,duration:.8,ease:'power2.inOut'},56.3);
 s.timeline.to(s.$('#ec-rax'),{left:80,top:550,width:300,duration:1.1,ease:'power2.inOut'},57.2);
 text(s,'#ec-rax .reg-detail','inspect返回PID',57.2);
 for(const id of ['ec-inspect-case','ec-user','ec-kernel','ec-main-code','ec-wrapper','ec-opcode'])hide(s,id,60);
 s.timeline.to(s.$('#ec-execution'),{autoAlpha:0,duration:.25},60);
 show(s,'ec-block-case',60.6);show(s,'ec-parent',61);show(s,'ec-helper',61.5);show(s,'ec-receiver',62);show(s,'ec-sender',62.4);show(s,'ec-pipe-title',62.7);show(s,'ec-pipe',62.7);
 text(s,'#ec-parent .ec-process-state','S / 阻塞在空管道read',64);show(s,'ec-block-meaning',64.5);show(s,'ec-block-owner',65);
 text(s,'#ec-helper .ec-process-state','收到!后运行，准备写K',68);text(s,'#ec-block-owner','代表性调度：省略其他任务；!只控制时机，不是K。',68);flow(s,'ec-schedule-helper',68.5,1.1);
 flow(s,'ec-copy',71,1.7);text(s,'#ec-receiver b',byte(result.byte),73);text(s,'#ec-parent .ec-process-state','read返回1 / 用户代码继续',73);flow(s,'ec-schedule-parent',73.2,1.1);
 text(s,'#ec-helper .ec-process-state','已退出，并被父进程回收',74);show(s,'ec-os-result',74);
 text(s,'#ec-os-result',`blocking / read=${result.readResult}；nvcsw ${result.nvBefore}→${result.nvAfter}；helper已回收`,74);
 for(const id of ['ec-block-case','ec-parent','ec-helper','ec-sender','ec-pipe-title','ec-pipe','ec-block-meaning','ec-block-owner'])hide(s,id,78);
 text(s,'#ec-receiver strong','blocking记录 / received ·1B',78.5);
 s.timeline.to(s.$('#ec-receiver'),{top:225,width:440,duration:.8,ease:'power2.inOut'},78.5);
 s.timeline.to(s.$('#ec-os-result'),{left:950,width:390,duration:.4,ease:'power2.inOut'},78.1);
 s.timeline.to(s.$('#ec-os-result'),{top:290,duration:.5,ease:'power2.inOut'},78.5);
 text(s,'#ec-os-result',`blocking / read=1 · nv ${result.nvBefore}→${result.nvAfter}\nhelper已回收，程序exit0`,78.5);
 show(s,'ec-fault-case',79.15);show(s,'ec-guard',79.25);show(s,'ec-fault-rip',79.1);
 text(s,'#ec-guard .array-value','---',83);text(s,'#ec-fault-rip .reg-value',faultBefore.registers.rip,85);
 flow(s,'ec-fault-request',85.1,1);show(s,'ec-signal',86.8);flow(s,'ec-fault-enter',87,2.4);
 text(s,'#ec-signal .reg-value',faultSignal.signal.name,90);s.timeline.set(s.$('#ec-signal .reg-value'),{color:COLORS.red},90);text(s,'#ec-fault-rip .reg-value',faultSignal.registers.rip,90);
 hide(s,'ec-guard',96);
 s.timeline.to(s.$('#ec-signal'),{left:950,top:365,width:390,duration:1,ease:'power2.inOut'},96.4);
 text(s,'#ec-fault-case',`fault记录 / PID ${faultReady.pid}`,96.4);
 text(s,'#ec-fault-rip .reg-detail','fault归档：故障前 = 信号时',96.4);
 show(s,'ec-irq-device',98);show(s,'ec-irq-handler',98.8);flow(s,'ec-irq',100,2);flow(s,'ec-irq-return',104,1.5);
 hide(s,'ec-irq-device',110);hide(s,'ec-irq-handler',110);
 finish(s.timeline,124);return s;
}
function local(id,duration,eyebrow,title,body,stops,checks,animate){const s=shot(id,`<div class="camera">${heading(id+'-head',80,90,eyebrow,title)}${body}</div>`,duration,{stops,checks});s.root.classList.add('control-flow-scene');show(s,id+'-head',.05);animate?.(s);return closeShot(s,{exit:false});}
const scenes=[
 {title:'真实C与用户态观察',view:'C / THREE SEPARATE RUNS',build:()=>local('ec-program',12,'USER OBSERVATIONS / KERNEL MECHANISM','先区分控制转移，再看是否换进程。',pos('ec-program-note',450,510,'完整C有inspect、blocking、fault三个独立模式。<br>用户寄存器／结果实测；内核入口与硬件中断是机制解释。','ec-short',{width:880}),[8,11],[{at:8,selector:'#ec-rip .reg-value',text:ready.registers.rip}],s=>show(s,'ec-program-note',6))},
 {title:'普通CALL仍在用户态',view:'USER CALL / SAME PROCESS',build:()=>local('ec-function',14,'CALL ≠ ENTER KERNEL','普通函数调用，只改变用户控制流。',pos('ec-call-note',450,480,'CALL进入local_add；返回值5与返回位置不是一回事。<br>这一步不需要进入内核，也不等于换进程。','ec-short',{width:880}),[4,10,12,13],[{at:4,selector:'#ec-rip .reg-value',text:entered.registers.rip},{at:10,selector:'#ec-rax .reg-value',text:dec(returned.registers.rax)},{at:12,selector:'#ec-sum .reg-value',text:'5'}],s=>show(s,'ec-call-note',5))},
 {title:'SYSCALL才是内核入口',view:'ACTUAL 0F05 / MODE ENTRY MODEL',build:()=>local('ec-syscall',18,'WRAPPER CALL ≠ SYSCALL INSTRUCTION','C包装函数里面，才执行SYSCALL。',pos('ec-syscall-note',450,398,`getpid无参数；调用号RAX=${before.fields.syscall_number}。`,'ec-short',{width:500}),[6,14,17],[{at:6,selector:'#ec-rip .reg-value',text:before.registers.rip},{at:6,selector:'#ec-rax .reg-value',text:dec(before.registers.rax)}],s=>show(s,'ec-syscall-note',5.5))},
 {title:'返回原进程，不证明从未调度',view:'USER RETURN / NO KERNEL PC CLAIM',build:()=>local('ec-sysreturn',16,'SYSCALL RETURN / SAME PID','进入内核，不等于必定切换进程。',pos('ec-return-note',450,505,`边界实测：RIP推进${before.instruction.length}B，RAX返回PID ${after.pid}。<br>前后同PID不证明中途从未被抢占；内核RIP未采集。`,'ec-short',{width:880})+pos('ec-pid-store',480,350,'C对象observed_pid = 0（写回前）','ec-short',{width:570}),[7,11,15],[{at:7,selector:'#ec-rip .reg-value',text:after.registers.rip},{at:11,selector:'#ec-rax .reg-value',text:dec(pidStored.registers.rax)}],s=>{show(s,'ec-return-note',7.4);show(s,'ec-pid-store',10);text(s,'#ec-pid-store',`C对象observed_pid = ${pidStored.fields.observed_pid}`,11);})},
 {title:'独立read阻塞与唤醒实验',view:'NON-GDB / PROC + RUSAGE',build:()=>local('ec-blocking',18,'INDEPENDENT OS RUN / EMPTY PIPE','read阻塞后，才有这个调度对照。','',[5,9,13,15,17],[{at:5,selector:'#ec-parent .ec-process-state',text:'S / 阻塞在空管道read'},{at:13,selector:'#ec-receiver b',text:byte(result.byte)}])},
 {title:'故障停在原指令，没有读回字节',view:'INDEPENDENT FAULT / ACTUAL SIGSEGV',build:()=>local('ec-fault',18,'SYNCHRONOUS FAULT / NO READ RESULT','本次保护页不可读：故障，而非正常返回。',source('ec-fault-c',420,365,['fault_value = fault_load(guard_page);'],500)+pos('ec-fault-ins',450,410,`${escape(faultBefore.instruction.asm)} / 读取保护页`,'ec-strong',{width:480})+pos('ec-fault-note',450,700,'本次SIGSEGV / ACCERR；si_addr等于guard_page。<br>同步故障不等于信号本身；合法缺页也可能被修复并重试。','ec-short',{width:890}),[2,7,12,17],[{at:2,selector:'#ec-fault-rip .reg-value',text:faultReady.registers.rip},{at:7,selector:'#ec-fault-rip .reg-value',text:faultBefore.registers.rip},{at:12,selector:'#ec-signal .reg-value',text:'SIGSEGV'}],s=>{show(s,'ec-fault-c',3);show(s,'ec-fault-ins',6.2);text(s,'#ec-fault-ins','访问被拒绝，未得到字节',12);show(s,'ec-fault-note',12.5);})},
 {title:'异步中断仅作机制对照',view:'MECHANISM ONLY / NO IRQ TRACE',build:()=>local('ec-interrupt',14,'ASYNCHRONOUS EXTERNAL EVENT / MODEL','中断：不一定由当前指令引起。',pos('ec-irq-note',450,700,'设备中断为机制示意，没有伪造硬件IRQ记录。<br>管道写入的唤醒是软件操作，不是本图的设备中断。','ec-short',{width:890}),[7,11,13],[],s=>show(s,'ec-irq-note',6))},
 {title:'内核入口与上下文切换是两个维度',view:'KEEP ORIGINAL EVIDENCE / TWO QUESTIONS',build:()=>local('ec-summary',14,'WHY ENTER KERNEL? / WHO RUNS NEXT?','两个问题，不要画等号。',pos('ec-final-call',450,370,'普通CALL：用户态内部转移','ec-strong',{width:420})+pos('ec-final-entry',450,425,'系统调用／故障／中断<br>进入内核的原因不同','ec-short',{width:420})+pos('ec-final-switch',450,510,'上下文切换：另选一个执行上下文','ec-formula',{width:890})+pos('ec-final-note',450,700,'本次read有实际阻塞与计数变化。<br>系统调用或中断，并不必然换进程；信号专题后续单独讲。','ec-short',{width:890}),[8,13],[{at:12,selector:'#ec-receiver b',text:byte(result.byte)},{at:12,selector:'#ec-signal .reg-value',text:'SIGSEGV'}],s=>{show(s,'ec-final-call',.8);show(s,'ec-final-entry',2);show(s,'ec-final-switch',4);show(s,'ec-final-note',7);})}
];
mountFilm({id:'exception-control',chapter:8,title:'异常、中断、系统调用与上下文切换',example:'../experiments/control-flow.c',model:'Linux x86-64 · 用户状态/GDB与独立OS观察 · 内核/IRQ/调度箭头是模型 · 各运行身份分开 · 无实时内核调试',
 provenance:{...t.provenance,samples:controlSummary.stops,events:controlSummary.stops,sourceHref:'../experiments/control-flow.c',jsonHref:'../traces/control-flow.json',gdbHref:'../traces/control-flow-gdb.txt'},experiment,buildContinuity:retained,scenes});attachControlLab(t);
