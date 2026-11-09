import assert from 'node:assert/strict';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {resolve} from 'node:path';
import {createHash} from 'node:crypto';
import {chromium} from 'playwright';
import {createPreviewServer} from '../preview-server.mjs';
import {reviewVisibility} from './film-visibility.mjs';
const root=resolve(import.meta.dirname,'..'),raw=JSON.parse(await readFile(resolve(root,'traces/control-flow.json'),'utf8'));
const at=(r,e)=>raw.runs[r].samples.find(s=>s.event===e),o=raw.os.blocking,br=o.records[0],helper=o.records[1],done=o.records[2];
const hex=n=>'0x'+BigInt(n).toString(16).toUpperCase(),decimal=n=>BigInt(n).toString(),sha=b=>createHash('sha256').update(b).digest('hex');
let checks=0;const issues=[],errors=[],reports=[];
const eq=(a,b,m)=>{assert.deepEqual(a,b,m);checks++;},ok=(a,m)=>{assert.ok(a,m);checks++;};
const seek=(p,time)=>p.evaluate(t=>film.seek(t),time);
async function shown(p,id){return p.locator('#'+id).evaluate(e=>{for(let n=e;n&&n.id!=='film-stage';n=n.parentElement){const s=getComputedStyle(n);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)<.999)return false;}return true;});}
const cues={inspect:{ready:8,'function-entry':16,'function-return':22,'sum-stored':24,'syscall-before':32,'syscall-after':51,'pid-stored':55},fault:{'fault-ready':80,'fault-before':85,'fault-signal':90}};
const cuts={12:['ec-cpu','ec-user','ec-rip','ec-rax','ec-sum'],26:['ec-cpu','ec-user','ec-rip','ec-rax','ec-sum'],44:['ec-cpu','ec-user','ec-kernel','ec-rip','ec-rax','ec-sum'],60:['ec-cpu','ec-rax','ec-sum'],78:['ec-cpu','ec-rax','ec-sum','ec-receiver','ec-os-result'],96:['ec-cpu','ec-rax','ec-sum','ec-receiver','ec-os-result','ec-signal','ec-fault-rip'],110:['ec-cpu','ec-rax','ec-sum','ec-receiver','ec-os-result','ec-signal','ec-fault-rip']};
const browser=await chromium.launch();await mkdir(resolve(root,'film-artifacts'),{recursive:true});
try{
 for(const published of process.env.CONTROL_SOURCE_ONLY?[false]:[false,true]){
  const server=createPreviewServer(published?{directory:resolve(root,'dist'),prefix:'/csapp'}:{});await new Promise(r=>server.listen(0,'127.0.0.1',r));
  const origin=`http://127.0.0.1:${server.address().port}`,base=origin+(published?'/csapp':'');
  try{
   const p=await browser.newPage({viewport:{width:1600,height:1160}});p.on('pageerror',e=>errors.push(e.message));p.on('response',r=>{if(r.status()>=400)errors.push(r.status()+' '+r.url());});p.on('console',m=>{if(m.type()==='error')errors.push(m.text());});p.on('request',r=>{if(!r.url().startsWith(origin)&&!r.url().startsWith('data:'))errors.push('Remote '+r.url());});
   await p.goto(base+'/chapter8/exception-control.html?paused');await p.waitForFunction(()=>window.film);
   eq(await p.evaluate(()=>[film.duration,film.chapters.length,film.dataSource,film.continuity.rootId]),[124,8,'gdb-plus-model','control-flow-retained'],'scope / eight independent local compositions');
   eq((await p.locator('#experiment-source > span code').allTextContents()).join('\n'),raw.provenance.source.content,'unmodified complete C');
   await p.evaluate(()=>{const root=document.querySelector('[data-film-continuity]');window.originalControl=[root,...root.querySelectorAll('*')];window.controlReplacements=[];new MutationObserver(rs=>{for(const r of rs)if([...r.addedNodes,...r.removedNodes].some(n=>n.nodeType===1))controlReplacements.push(r.target.id||r.target.className);}).observe(root,{childList:true,subtree:true});});
   for(const[time,id]of [[0,'ec-cpu'],[12,'ec-callee'],[26,'ec-wrapper'],[33.9,'ec-kernel'],[60,'ec-parent'],[78,'ec-guard'],[86.7,'ec-signal'],[97.9,'ec-irq-device']]){await seek(p,time);eq(await shown(p,id),false,'future object not leaked '+id);}
   for(const[run,points]of Object.entries(cues))for(const[event,time]of Object.entries(points)){
    const s=at(run,event);await seek(p,time);
    if(run==='inspect'){
     ok(await shown(p,'ec-rip')&&await shown(p,'ec-rax'),'visible actual user register snapshots at cue');
     eq(await p.locator('#ec-rip .reg-value').textContent(),s.registers.rip,'independent raw user RIP');eq(await p.locator('#ec-rax .reg-value').textContent(),decimal(s.registers.rax),'independent raw user RAX');eq(await p.locator('#ec-sum .reg-value').textContent(),String(s.fields.observed_sum),'C global store is later than CALL result');
    }else{
     ok(await shown(p,'ec-fault-rip')&&await shown(p,'ec-guard'),'fault observation fully visible at cue');eq(await p.locator('#ec-fault-rip .reg-value').textContent(),s.registers.rip,'fault checkpoint / pre-instruction / signal RIP');eq(await p.locator('#ec-guard .array-address').textContent(),s.guardAddress,'actual full guard VA');eq(await p.locator('#ec-guard .array-value').textContent(),event==='fault-ready'?'RW':'---','permission state, not a returned data byte');
     if(event==='fault-signal')eq(await p.locator('#ec-signal .reg-value').textContent(),s.signal.name,'actual signal only after failed instruction');
    }
   }
   await seek(p,54.99);ok((await p.locator('#ec-pid-store').textContent()).includes('= 0'),'no future global PID store');await seek(p,55);ok((await p.locator('#ec-pid-store').textContent()).includes(String(at('inspect','pid-stored').fields.observed_pid)),'actual C PID assignment');
   for(const[time,expected]of [[63.3,'CC'],[72.99,'CC'],[73.01,'4B'],[95,'4B'],[123,'4B']]){await seek(p,time);eq(await p.locator('#ec-receiver b').textContent(),expected,'one byte commits after arrival');eq(await p.locator('#ec-receiver').getAttribute('data-capacity'),'1','fixed1B target');eq(await p.locator('#ec-receiver').getAttribute('data-run'),'blocking','original run identity kept');}
   await seek(p,75);eq(await p.locator('#ec-sender b').textContent(),"4B ('K')",'source preserved');eq(await p.locator('#ec-parent b').textContent(),'PID '+br.pid,'real blocking parent');eq(await p.locator('#ec-helper b').textContent(),'PID '+helper.pid,'separate actual helper');ok((await p.locator('#ec-os-result').textContent()).includes(`${done.nvBefore}→${done.nvAfter}`),'actual rusage counts');
   eq(await p.locator('#ec-guard .array-row').count(),1,'one fixed mapping, not an enlarged buffer');ok((await p.locator('#ec-guard').textContent()).includes(String(at('fault','fault-ready').fields.page_size)+'B'),'actual page capacity');
   const fingerprints=[];
   for(const[time,ids]of Object.entries(cuts)){
    const states=[];
    for(const delta of[-.001,.001]){await seek(p,Number(time)+delta);states.push(await p.evaluate(ids=>ids.map(id=>{const e=document.getElementById(id),r=e.getBoundingClientRect();let visible=true;for(let n=e;n&&n.id!=='film-stage';n=n.parentElement){const s=getComputedStyle(n);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)<.999)visible=false;}return{visible,x:r.x,y:r.y,data:[...e.querySelectorAll('.reg-value,.array-value,.array-address,b')].map(n=>n.textContent)};}),ids));}
    ids.forEach((id,i)=>{ok(states[0][i].visible&&states[1][i].visible,'no clear/fade at cut '+id);eq(states[0][i].data,states[1][i].data,'same measured data across cut');ok(Math.hypot(states[0][i].x-states[1][i].x,states[0][i].y-states[1][i].y)<2,'continuous position at cut');});
   }
   const signature=()=>p.evaluate(()=>[...document.querySelectorAll('[data-film-continuity] .positioned')].map(e=>{const r=e.getBoundingClientRect(),s=getComputedStyle(e);return[e.id,e.textContent,s.opacity,s.visibility,r.x.toFixed(2),r.y.toFixed(2),r.width.toFixed(2)];}));
   const times=[0,8,16,24,32,51,55,63,75,80,85,90,103,123,71.8,57.7];for(const time of times){await seek(p,time);fingerprints.push(await signature());}for(let i=times.length-1;i>=0;i--){await seek(p,times[i]);eq(await signature(),fingerprints[i],'reverse exact text / visibility / layout');ok(await p.evaluate(()=>originalControl.every(n=>document.contains(n))),'all original actor elements retained');}
   const points=[...new Set([...Array.from({length:310},(_,i)=>.08+i*.4),...Object.keys(cuts).flatMap(t=>[-.3,0,.15,.35,.55,.8,1.2,1.7,2.4].map(d=>Number(t)+d)),...[14.6,20.8,23,37,49,57.8,68.8,71.7,73.8,78.8,87.6,100.8,104.6].flatMap(t=>[0,.2,.4,.6,.85].map(d=>t+d))])].filter(t=>t>=0&&t<124);
   for(const time of points){await seek(p,time);const result=await reviewVisibility(p);if(Object.values(result).some(a=>a.length))issues.push({variant:published?'published':'source',time,result});checks++;}
   for(const[time,ids]of Object.entries(cuts)){
    const boundary=Number(time);await seek(p,boundary-.22);
    const frames=await p.evaluate(async({boundary,ids})=>{const saved=ids.map(id=>document.getElementById(id)),rows=[];film.play();await new Promise(resolve=>{function tick(){const time=film.getState().time;rows.push({time,valid:ids.every((id,i)=>{let e=document.getElementById(id);if(e!==saved[i])return false;for(;e&&e.id!=='film-stage';e=e.parentElement){const s=getComputedStyle(e);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)!==1)return false;}return true;})});if(time>boundary+.28){film.pause();resolve();}else requestAnimationFrame(tick);}requestAnimationFrame(tick);});return rows;},{boundary,ids});
    ok(frames.length>4&&frames.some(r=>r.time<boundary)&&frames.some(r=>r.time>=boundary),'real rAF frames across both sides of cut');ok(frames.every(r=>r.valid),'same visible actors during actual playback');
   }
   for(const[id,a,b]of[['ec-execution',36.3,37.5],['ec-copy-packet',71.2,72.1],['ec-fault-enter-packet',87.3,88.5]]){const positions=[];for(const time of[a,b]){await seek(p,time);ok(await shown(p,id),'actual moving packet visible');positions.push(await p.locator('#'+id).evaluate(n=>{const r=n.getBoundingClientRect();return[r.x,r.y];}));}ok(Math.hypot(positions[0][0]-positions[1][0],positions[0][1]-positions[1][1])>12,'real path displacement');}
   eq(await p.evaluate(()=>controlReplacements),[],'no cloned replacements / no deleted original children');
   const actualCues=await p.evaluate(()=>film.experimentCues);eq(actualCues.length,10,'all ten GDB observations');
   for(const[run,r]of Object.entries(raw.runs)){await p.selectOption('#experiment-run',run);for(const s of r.samples){await p.selectOption('#experiment-stop',String(s.index));eq(await p.locator('#experiment-command-output').textContent(),s.commands.map(c=>'(gdb) '+c.command+'\n'+c.output).join('\n\n'),'original GDB commands / output');const observation=JSON.parse(await p.locator('#control-observation').textContent());eq(observation.registers,s.registers,'all actual registers in selected observation');eq(observation.initializedFields,s.fields,'all initialized C fields');eq(observation.run,run,'no execution identity merge');await p.locator('#experiment-locate').click();eq(await p.evaluate(()=>film.getState().time),cues[run][s.event],'honest observed-state location');}}
   eq(await p.locator('#control-os-program').textContent(),o.command+'\nexitCode='+o.exitCode+'\n'+o.stdout,'actual independent program stdout');eq(await p.locator('#control-os-files').textContent(),o.beforeRelease.files.map(f=>f.operation+' '+f.path+'\n'+f.content).join('\n\n'),'original proc readFile data, not invented shell commands');await p.locator('#control-os-locate').click();eq(await p.evaluate(()=>film.getState().time),75,'independent OS locate');
   for(const[path,hash]of[[raw.provenance.source.path,raw.provenance.source.sha256],[raw.provenance.binary.path,raw.provenance.binary.sha256]]){const r=await p.request.get(base+'/'+path);eq(r.status(),200,'controlled download');eq(sha(await r.body()),hash,'independent artifact hash');}
   for(const a of await p.locator('.experiment-links a').all())eq((await p.request.get(new URL(await a.getAttribute('href'),p.url()).href)).status(),200,'offline evidence / guide links');
   if(!published){for(const[title,time]of[['syscall',37.6],['block',71.8],['fault',88],['transition',78.9]]){await seek(p,time);await p.locator('.film-viewport').screenshot({path:resolve(root,'film-artifacts','control-'+title+'-action.png')});}await p.locator('#control-os-program').scrollIntoViewIfNeeded();await p.waitForTimeout(160);ok(await p.locator('#control-os-program').evaluate(e=>getComputedStyle(e).color==='rgb(255, 255, 255)'&&e.getBoundingClientRect().height>0),'OS report visibly rendered in white');await p.screenshot({path:resolve(root,'film-artifacts/control-os-lab.png')});}
   await p.setViewportSize({width:390,height:850});ok(await p.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'390px C / GDB / OS lab no page overflow');await p.close();
   const g=await browser.newPage({viewport:{width:390,height:850}});await g.goto(base+'/chapter8/guide.html');ok(await g.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'guide fits390');ok((await g.locator('article').innerText()).includes('进入内核不等于必定切换进程'),'guide separates mode from scheduling');for(const a of await g.locator('article a').all())eq((await g.request.get(new URL(await a.getAttribute('href'),g.url()).href)).status(),200,'guide local links');await g.close();reports.push({variant:published?'published':'source',sampledTimes:points.length,cuts:Object.keys(cuts).length});
  }finally{await new Promise(r=>server.close(r));}
 }
 eq(errors,[],'no script/SVG errors / missing resources / remote runtime');await writeFile(resolve(root,'film-artifacts/control-flow-verification.json'),JSON.stringify({checks:checks+1,reports,issues,errors},null,2));
 if(!process.env.CONTROL_DIAGNOSTIC)eq(issues,[],'all persistent/local text, opaque panels and moving packets readable');else console.log('DIAGNOSTIC',issues.length,JSON.stringify(issues.slice(0,12)));
 console.log(`${process.env.CONTROL_DIAGNOSTIC?'DIAGNOSTIC':'PASS'} ${checks} independent control-flow / syscall / OS / fault / identity / actual-cut / motion checks`);
}finally{await browser.close();}
