import {spawn,spawnSync} from 'node:child_process';
import {readFile,writeFile,mkdir,rename} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {resolve} from 'node:path';
import {validateControlFlow,procFields} from '../models/validate-control-flow.js';
const root=resolve(import.meta.dirname,'..'),dir='.trace-build/control-flow',sourcePath='experiments/control-flow.c',compiler=process.env.CC||'gcc';
const flags=['-std=c11','-O0','-g3','-fPIE','-pie','-fno-omit-frame-pointer','-fno-optimize-sibling-calls','-Wall','-Wextra','-Wpedantic'];
function run(exe,args){const p=spawnSync(exe,args,{cwd:root,encoding:'utf8',timeout:45000,maxBuffer:24*1024*1024});if(p.error||p.status!==0)throw Error(`${exe} failed; evidence not published\n${p.error||''}\n${p.stdout}\n${p.stderr}`);return p.stdout;}
const sha=b=>createHash('sha256').update(b).digest('hex');
async function observeBlocking(){
 const child=spawn(resolve(root,dir,'program'),['blocking'],{cwd:root,stdio:['pipe','pipe','pipe']});
 let stdout='',stderr='',pending='',ready,resolveReady,rejectReady;
 const readyPromise=new Promise((r,j)=>{resolveReady=r;rejectReady=j;});
 const exited=new Promise((r,j)=>{child.on('error',j);child.on('exit',(code,signal)=>r({code,signal}));});
 const timer=setTimeout(()=>{child.stdin.end();child.kill('SIGKILL');rejectReady(Error('Reviewed blocking run timed out'));},16000);
 child.stdout.on('data',b=>{const text=b.toString();stdout+=text;pending+=text;let at;while((at=pending.indexOf('\n'))>=0){const line=pending.slice(0,at);pending=pending.slice(at+1);try{const item=JSON.parse(line);if(item.event==='blocking-ready'){ready=item;resolveReady(item);}}catch(e){rejectReady(e);}}});
 child.stderr.on('data',b=>stderr+=b.toString());
 try{
  await Promise.race([readyPromise,exited.then(()=>{if(!ready)throw Error('Program exited before ready');return ready;})]);
  let beforeRelease;
  for(let attempt=0;attempt<1000;attempt++){
   const paths=[`/proc/${ready.pid}/status`,`/proc/${ready.pid}/syscall`,`/proc/${ready.pid}/wchan`,`/proc/${ready.child}/status`];
   const files=await Promise.all(paths.map(async path=>({operation:'readFile',path,content:await readFile(path,'utf8')})));
   const ps=procFields(files[0].content),cs=procFields(files[3].content),args=files[1].content.trim().split(/\s+/);
   if(ps.State.startsWith('S')&&cs.State.startsWith('S')&&args[0]===String(ready.readNumber)&&args.length>=4&&BigInt(args[1])===BigInt(ready.fd)&&BigInt(args[2])===BigInt(ready.buffer)&&BigInt(args[3])===1n){beforeRelease={attempts:attempt+1,files};break;}
   await new Promise(r=>setTimeout(r,5));
  }
  if(!beforeRelease)throw Error('Could not observe the reviewed empty-pipe read; no guessed schedule');
  child.stdin.end('!');const end=await exited;
  // drain output streams, including the helper's inherited stdout descriptor
  await Promise.all([child.stdout,child.stderr].map(s=>s.readableEnded?Promise.resolve():new Promise(r=>s.on('end',r))));
  if(end.code!==0||end.signal||stderr)throw Error(`Blocking failed: ${JSON.stringify(end)} ${stderr}`);
  return{command:`${dir}/program blocking`,exitCode:end.code,stderr,stdout,records:stdout.trim().split('\n').map(JSON.parse),beforeRelease,release:'!',scope:'Independent non-GDB run. Only target processes pinned to one originally allowed CPU. /proc read before collector releases helper; rusage delta is not an exclusive schedule trace.'};
 }finally{clearTimeout(timer);if(child.exitCode===null)child.stdin.end();}
}
await mkdir(resolve(root,dir),{recursive:true});const command=[compiler,...flags,sourcePath,'-o',dir+'/program'];run(command[0],command.slice(1));
const session=run('gdb',['-nx','-q','-batch','-x','scripts/gdb-control-flow.py']);
await writeFile(resolve(root,dir,'session.log'),session);
const raw=JSON.parse(await readFile(resolve(root,dir,'raw.json'),'utf8')),blocking=await observeBlocking();
const content=await readFile(resolve(root,sourcePath),'utf8'),binary=await readFile(resolve(root,dir,'program'));
const trace={schemaVersion:1,id:'control-flow',provenance:{kind:'gdb-plus-model',capturedAt:new Date().toISOString(),source:{path:sourcePath,content,sha256:sha(content)},binary:{path:'traces/control-flow/program.elf',sha256:sha(binary)},compiler:run(compiler,['--version']).split('\n')[0],compilerCommand:command.join(' '),flags,gdb:'GNU gdb '+raw.gdb,architecture:raw.architecture,kernelASLR:(await readFile('/proc/sys/kernel/randomize_va_space','utf8')).trim(),debuggerASLR:raw.randomization,scope:'Actual user CALL/SYSCALL states and deliberate own-page SIGSEGV; separate non-GDB blocking read and /proc/rusage observations. Kernel entry, scheduling arrows, hardware interrupt and teaching time remain models, not live kernel/IRQ tracing.'},runs:raw.runs,os:{blocking},gdbSession:session};
const summary=validateControlFlow(trace);await mkdir(resolve(root,'traces/control-flow'),{recursive:true});
await writeFile(resolve(root,trace.provenance.binary.path),binary);
await writeFile(resolve(root,'traces/control-flow/blocking-os.txt'),`${blocking.command}\n${blocking.scope}\n\nstdout:\n${blocking.stdout}\nObserved before release=${JSON.stringify(blocking.release)}:\n`+blocking.beforeRelease.files.map(f=>`${f.operation} ${f.path}\n${f.content}`).join('\n\n'));
const target=resolve(root,'traces/control-flow.json');await writeFile(target+'.tmp',JSON.stringify(trace,null,2)+'\n');await rename(target+'.tmp',target);
await writeFile(resolve(root,'traces/control-flow-gdb.txt'),`Captured: ${trace.provenance.capturedAt}\nSource SHA256: ${trace.provenance.source.sha256}\nELF SHA256: ${trace.provenance.binary.sha256}\n${trace.provenance.compilerCommand}\n\n${raw.gdbText}\n\n=== Original GDB batch stdout (including stop/exit notifications) ===\n${session}`);
console.log(`PASS control-flow: ${summary.gdbRuns} GDB runs / ${summary.stops} stops; ${summary.osRuns} independent blocked-read OS run / observed rusage change`);
