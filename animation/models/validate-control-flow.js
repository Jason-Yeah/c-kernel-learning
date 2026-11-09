const fail=m=>{throw Error('Control-flow evidence rejected: '+m);};
const require=(yes,m)=>{if(!yes)fail(m);};
const address=v=>typeof v==='string'&&/^0x[0-9A-F]+$/i.test(v);
export function procFields(content){return Object.fromEntries(content.trim().split('\n').map(line=>{const at=line.indexOf(':');return[line.slice(0,at),line.slice(at+1).trim()];}));}
export function validateControlFlow(t){
 require(t?.schemaVersion===1&&t.id==='control-flow'&&t.provenance?.kind==='gdb-plus-model','schema');
 const p=t.provenance;require(p.source?.path==='experiments/control-flow.c'&&p.source.content.includes('int main(')&&p.source.content.includes('mprotect('),'complete reviewed source');
 require([p.source.sha256,p.binary?.sha256].every(h=>/^[a-f0-9]{64}$/.test(h)),'hashes');
 require(/x86-64/.test(p.architecture)&&p.kernelASLR==='2'&&/off/.test(p.debuggerASLR),'recorded platform / ASLR');
 const events={inspect:['ready','function-entry','function-return','sum-stored','syscall-before','syscall-after','pid-stored'],fault:['fault-ready','fault-before','fault-signal']};
 for(const[key,names]of Object.entries(events)){
  const r=t.runs?.[key];require(r?.samples?.length===names.length,key+' observations');let pid;
  r.samples.forEach((s,i)=>{
   require(s.index===i&&s.event===names[i]&&Number.isInteger(s.pid)&&s.pid>0&&(!pid||s.pid===pid),'event / execution identity');pid=s.pid;
   require(s.fields.input_a===3&&s.fields.input_b===2&&s.fields.syscall_number===39&&s.fields.read_syscall_number===0,'real fixed inputs / Linux ABI');
   require(s.fields.fault_read_complete===0&&s.fields.fault_value===0,'fault never produces a read result');
   require(s.source?.file===p.source.path&&Number.isInteger(s.source.line),'source location');
   require(address(s.guardAddress)&&s.instruction.address===s.registers.rip&&s.instruction.bytes.length===s.instruction.length,'instruction / address');
   const pointer=s.commands?.find(c=>c.command==='p/x guard_page')?.output;
   require(pointer&&pointer.toLowerCase().includes(s.guardAddress.toLowerCase()),'raw guard pointer');
   const registers=s.commands?.find(c=>c.command==='info registers rip rsp rbp rax rdi rsi rdx rcx r11 eflags cs')?.output;
   require(typeof registers==='string','raw register command');
   for(const[name,v]of Object.entries(s.registers)){
    const found=registers.match(new RegExp('^'+name+'\\s+(0x[0-9a-f]+)','im'));
    require(address(v)&&found&&BigInt(found[1])===BigInt(v),'raw register '+name);
   }
   require((BigInt(s.registers.cs)&3n)===3n,'GDB sees user privilege, not fake kernel CS');
   const bytes=s.commands.find(c=>c.command===`x/${s.instruction.length}bx $rip`)?.output.match(/\b0x[0-9a-f]{2}\b/gi)?.map(v=>parseInt(v,16));
   require(bytes?.length===s.instruction.bytes.length&&bytes.every((b,j)=>b===s.instruction.bytes[j]),'raw instruction bytes');
   for(const[name,v]of Object.entries(s.fields)){
    const output=s.commands.find(c=>c.command==='p/d '+name)?.output;
    require(Number.isSafeInteger(v)&&output&&Number(output.match(/=\s*(-?\d+)/)?.[1])===v,'raw initialized field '+name);
   }
   if(key==='inspect')require(s.guardAddress==='0x0'&&s.fields.page_size===0&&s.fields.observed_sum===(i>=3?5:0)&&s.fields.observed_pid===(i===6?s.pid:0),'inspect result lifetime');
   else{
    require(BigInt(s.guardAddress)>0n&&s.guardAddress===r.samples[0].guardAddress&&s.fields.page_size>=4096&&(s.fields.page_size&(s.fields.page_size-1))===0,'fixed own mapping');
    require(s.fields.observed_sum===0&&s.fields.observed_pid===0,'independent fault run');
    if(i===0){require(s.guardByte===165&&s.commands.some(c=>c.command==='x/1bx guard_page'&&/0xa5/i.test(c.output)),'observed initialization');}
    else{
     require(!('guardByte'in s)&&!s.commands.some(c=>c.command==='x/1bx guard_page'),'protected target not read by collector');
     const line=s.maps?.content.split('\n').map(l=>l.match(/^([0-9a-f]+)-([0-9a-f]+)\s+(\S+)/)).find(m=>m&&BigInt('0x'+m[1])<=BigInt(s.guardAddress)&&BigInt(s.guardAddress)<BigInt('0x'+m[2]));
     require(line&&line[3].startsWith('---'),'actual no-read mapping');
    }
   }
  });
 }
 const a=t.runs.inspect.samples,[ready,entry,returned,stored,before,after,last]=a;
 require(BigInt(entry.registers.rdi)===3n&&BigInt(entry.registers.rsi)===2n&&BigInt(returned.registers.rax)===5n,'ordinary function arguments / result');
 require(address(entry.returnAddress)&&returned.registers.rip===entry.returnAddress&&BigInt(returned.registers.rsp)===BigInt(entry.registers.rsp)+8n,'real CALL return target and stack restoration');
 require(before.instruction.bytes.join(',')==='15,5'&&before.instruction.length===2&&BigInt(before.registers.rax)===BigInt(before.fields.syscall_number),'SYSCALL before');
 require(BigInt(after.registers.rip)===BigInt(before.registers.rip)+2n&&after.registers.rip===after.registers.rcx&&after.registers.rsp===before.registers.rsp&&BigInt(after.registers.rax)===BigInt(before.pid),'SYSCALL user return');
 require(t.runs.inspect.outcome?.kind==='exit'&&t.runs.inspect.outcome.code===0&&JSON.parse(t.runs.inspect.stdout).pid===last.pid,'normal inspect exit');
 const f=t.runs.fault.samples;
 require(f[1].instruction.bytes.join(',')==='138,7'&&f[1].registers.rdi===f[1].guardAddress&&f[2].registers.rip===f[1].registers.rip,'same faulting MOV / no normal next RIP');
 require(f[2].signal?.number===11&&f[2].signal.name==='SIGSEGV'&&f[2].signal.code===2&&f[2].signal.address===f[1].guardAddress,'actual SIGSEGV / ACCERR / address');
 require(t.runs.fault.outcome?.kind==='signal'&&t.runs.fault.outcome.signal==='SIGSEGV'&&t.gdbSession?.includes('Program terminated with signal SIGSEGV'),'signal outcome, not normal exit');
 const signalAddress=f[2].commands.find(c=>c.command==='p/x $_siginfo.si_addr')?.output;
 require(signalAddress&&signalAddress.toLowerCase().includes(f[2].signal.address.toLowerCase()),'raw signal address');
 const o=t.os?.blocking,records=o?.records;
 require(o?.exitCode===0&&o.stderr===''&&records?.length===3,'independent OS run');
 const br=records.find(r=>r.event==='blocking-ready'),helper=records.find(r=>r.event==='helper-write'),result=records.find(r=>r.event==='blocking-result');
 require(br&&helper&&result&&br.pid===result.pid&&br.child===helper.pid&&helper.ppid===br.pid,'real OS identities');
 require(br.capacity===1&&br.beforeByte===204&&result.capacity===1&&result.readResult===1&&result.byte===75&&helper.byte===75,'fixed one-byte copy / actual read result');
 require(br.cpu===helper.cpu&&br.cpu===result.cpu&&Number.isInteger(br.cpu)&&br.cpu>=0,'target-only one-CPU affinity');
 require(result.nvAfter>result.nvBefore&&result.nivAfter>=result.nivBefore&&result.waited===helper.pid&&result.status===0,'actual rusage change / child reaped');
 require(o.release==='!'&&o.beforeRelease?.files?.length===4,'observation before release');
 const files=o.beforeRelease.files,ps=files.find(f=>f.path===`/proc/${br.pid}/status`),cs=files.find(f=>f.path===`/proc/${br.child}/status`),sc=files.find(f=>f.path===`/proc/${br.pid}/syscall`);
 require(ps&&cs&&sc&&files.every(f=>f.operation==='readFile'&&typeof f.content==='string'),'original proc reads');
 const parent=procFields(ps.content),child=procFields(cs.content),tokens=sc.content.trim().split(/\s+/);
 require(parent.State.startsWith('S')&&child.State.startsWith('S')&&Number(child.PPid)===br.pid,'observed blocked parent / controlled helper');
 require(parent.Cpus_allowed_list===String(br.cpu)&&child.Cpus_allowed_list===String(br.cpu),'actual inherited allowed mask');
 require(Number(tokens[0])===br.readNumber&&br.readNumber===0&&BigInt(tokens[1])===BigInt(br.fd)&&BigInt(tokens[2])===BigInt(br.buffer)&&BigInt(tokens[3])===1n,'parent really blocked in its data-pipe read');
 require(o.stdout.trim().split('\n').length===3&&o.stdout.trim().split('\n').every((line,i)=>JSON.stringify(JSON.parse(line))===JSON.stringify(records[i])),'original program stdout');
 return{gdbRuns:2,stops:10,osRuns:1};
}
