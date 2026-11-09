import {escape} from './scene-kit.js';
export function attachControlLab(trace){
 const o=trace.os.blocking;
 document.querySelector('.experiment-links').insertAdjacentHTML('beforeend','<a href="../traces/control-flow/program.elf">实际ELF ↗</a><a href="../traces/control-flow/blocking-os.txt">独立OS原始记录 ↗</a><a href="./guide.html">异常控制流讲义 ↗</a>');
 const box=document.createElement('details');box.id='control-os-lab';box.className='control-os-lab';box.open=true;
 box.innerHTML=`<summary>独立OS观察：阻塞read、受控唤醒与上下文切换计数</summary><p>这不是GDB运行，也不是完整调度轨迹。只把本实验及helper限制到原allowed mask中的一个逻辑CPU，不独占CPU，不改宿主配置。下列/proc读取都发生在collector发出控制字符!之前；!不是业务字节K。</p><p><button id="control-os-locate" type="button">定位阻塞读取的实测结果</button></p><h3>原程序命令与stdout</h3><pre id="control-os-program">${escape(o.command+'\nexitCode='+o.exitCode+'\n'+o.stdout)}</pre><h3>释放helper之前的原始文件读取</h3><pre id="control-os-files">${escape(o.beforeRelease.files.map(f=>f.operation+' '+f.path+'\n'+f.content).join('\n\n'))}</pre><p>rusage计数差是该窗口的观察值，可能包含其他调度影响；不能据此推断恰好只有一次P→helper→P。wchan为原读值，若不可见不补造名称。目标程序回收helper并正常退出。</p><h3>所选GDB停点的架构状态／结果</h3><pre id="control-observation"></pre>`;
 document.querySelector('#experiment-workbench').after(box);
 document.querySelector('#control-os-locate').addEventListener('click',()=>{window.film.seek(75);document.querySelector('.player').scrollIntoView({block:'start',behavior:'instant'});});
 const update=()=>{const run=document.querySelector('#experiment-run').value,s=trace.runs[run].samples[Number(document.querySelector('#experiment-stop').value)];document.querySelector('#control-observation').textContent=JSON.stringify({run,event:s.event,pid:s.pid,instruction:s.instruction,registers:s.registers,initializedFields:s.fields,guardAddress:s.guardAddress,signal:s.signal||null,outcome:trace.runs[run].outcome},null,2);};
 document.querySelector('#experiment-run').addEventListener('change',update);document.querySelector('#experiment-stop').addEventListener('change',update);update();
}
