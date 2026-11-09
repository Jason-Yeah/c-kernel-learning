import {spawnSync} from 'node:child_process';
import {readFile,writeFile,mkdir,rename} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {resolve} from 'node:path';
import {validateIntegerBits} from '../models/validate-integer-bits.js';
const root=resolve(import.meta.dirname,'..'), dir='.trace-build/integer-bits';
const sourcePath='experiments/integer-bits.c', compiler=process.env.CC||'gcc';
const flags=['-std=c11','-O0','-g3','-fPIE','-pie','-fno-omit-frame-pointer','-Wall','-Wextra','-Wpedantic'];
function run(exe,args){const p=spawnSync(exe,args,{cwd:root,encoding:'utf8',timeout:45000,maxBuffer:24*1024*1024});if(p.error||p.status!==0)throw Error(`${exe} failed; trace not published\n${p.error||''}\n${p.stdout}\n${p.stderr}`);return p;}
const sha=data=>createHash('sha256').update(data).digest('hex');
await mkdir(resolve(root,dir),{recursive:true});
const command=[compiler,...flags,sourcePath,'-o',dir+'/program'];run(command[0],command.slice(1));
const sanitizerCommand=[compiler,...flags,'-fsanitize=undefined','-fno-sanitize-recover=undefined',sourcePath,'-o',dir+'/sanitized'];run(sanitizerCommand[0],sanitizerCommand.slice(1));
const sanitized=run('./'+dir+'/sanitized',[]);
await writeFile(resolve(root,dir,'session.log'),run('gdb',['-nx','-q','-batch','-x','scripts/gdb-integer-bits.py']).stdout);
const raw=JSON.parse(await readFile(resolve(root,dir,'raw.json'),'utf8'));
const content=await readFile(resolve(root,sourcePath),'utf8'),binary=await readFile(resolve(root,dir,'program'));
const trace={schemaVersion:1,id:'integer-bits',provenance:{kind:'gdb-plus-model',capturedAt:new Date().toISOString(),
 source:{path:sourcePath,content,sha256:sha(content)},binary:{path:'traces/integer-bits/program.elf',sha256:sha(binary)},
 compiler:run(compiler,['--version']).stdout.split('\n')[0],compilerCommand:command.join(' '),flags,
 gdb:'GNU gdb '+raw.gdb,architecture:raw.architecture,kernelASLR:(await readFile('/proc/sys/kernel/randomize_va_space','utf8')).trim(),debuggerASLR:raw.randomization,
 scope:'One reviewed C11 execution: actual fixed-width values, promotions and object bytes. Per-bit derivation and motion are mathematical explanation, not CPU timing. Invalid shifts / int32 signed overflow are guarded and never executed.'},
 types:raw.types,runs:{single:{samples:raw.samples}},exitCode:raw.exitCode,stdout:raw.stdout,
 sanitizer:{command:sanitizerCommand.join(' '),runCommand:'./'+dir+'/sanitized',binarySHA256:sha(await readFile(resolve(root,dir,'sanitized'))),exitCode:sanitized.status,stdout:sanitized.stdout,stderr:sanitized.stderr}};
const summary=validateIntegerBits(trace);
await mkdir(resolve(root,'traces/integer-bits'),{recursive:true});
await writeFile(resolve(root,trace.provenance.binary.path),binary);
await writeFile(resolve(root,'traces/integer-bits/sanitizer.txt'),`${trace.sanitizer.command}\n${trace.sanitizer.runCommand}\nexitCode=${trace.sanitizer.exitCode}\nstdout:\n${trace.sanitizer.stdout}\nstderr:\n${trace.sanitizer.stderr}`);
const target=resolve(root,'traces/integer-bits.json');await writeFile(target+'.tmp',JSON.stringify(trace,null,2)+'\n');await rename(target+'.tmp',target);
await writeFile(resolve(root,'traces/integer-bits-gdb.txt'),`Captured: ${trace.provenance.capturedAt}\nSource SHA256: ${trace.provenance.source.sha256}\nELF SHA256: ${trace.provenance.binary.sha256}\n${trace.provenance.compilerCommand}\n\n${raw.gdbText}`);
console.log(`PASS integer-bits: ${summary.samples} actual GDB stops / ${summary.objects} typed objects; same-source UBSan clean exit0`);
