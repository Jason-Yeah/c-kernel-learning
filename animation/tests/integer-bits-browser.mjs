import assert from 'node:assert/strict';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {resolve} from 'node:path';
import {chromium} from 'playwright';
import {createPreviewServer} from '../preview-server.mjs';
import {reviewVisibility} from './film-visibility.mjs';
const root=resolve(import.meta.dirname,'..'),raw=JSON.parse(await readFile(resolve(root,'traces/integer-bits.json'),'utf8'));
const ss=raw.runs.single.samples,at=e=>ss.find(s=>s.event===e),v=(e,n)=>at(e).fields[n];
const hex=(n,w=32)=>'0x'+BigInt.asUintN(w,BigInt(n)).toString(16).toUpperCase().padStart(w/4,'0');
const bin=(n,w)=>BigInt.asUintN(w,BigInt(n)).toString(2).padStart(w,'0');
const sha=b=>createHash('sha256').update(b).digest('hex');
let checks=0;const errors=[],issues=[],films=[];
const eq=(a,b,m)=>{assert.deepEqual(a,b,m);checks++;},ok=(v,m)=>{assert.ok(v,m);checks++;};
const seek=(p,t)=>p.evaluate(t=>film.seek(t),t);
async function shown(p,id){return p.locator('#'+id).evaluate(el=>{for(let n=el;n&&n.id!=='film-stage';n=n.parentElement){const s=getComputedStyle(n);if(s.visibility==='hidden'||s.display==='none'||Number(s.opacity)<.999)return false;}return true;});}
async function assertWord(p,id,n){
 eq(await p.locator('#'+id).getAttribute('data-bit-width'),'32','fixed32 capacity');
 eq(await p.locator('#'+id+' .ibit-cell').count(),8,'eight expanded bits plus explicitly folded24');
 eq(await p.locator('#'+id+' .ibit-full').textContent(),hex(n),'full32, not only low byte');
 eq((await p.locator('#'+id+' .bit-value').allTextContents()).join(''),bin(n,32).slice(-8),'low8 actual bits');
 eq(await p.locator('#'+id+' .ibit-fold-value').textContent(),'0x'+(BigInt.asUintN(32,BigInt(n))>>8n).toString(16).toUpperCase().padStart(6,'0'),'actual high24');
}
async function assertRibbon(p,id,n,w){eq(await p.locator('#'+id).getAttribute('data-bit-width'),String(w),'logical capacity remains fixed through folding');eq(await p.locator('#'+id+' .bit-value').count(),w,'no bit added or removed');eq(await p.locator('#'+id+' .ibit-ribbon-hex').textContent(),hex(n,w),'actual typed encoding');eq((await p.locator('#'+id+' .bit-value').allTextContents()).join(''),bin(n,w),'every bit of measured object');}
const plans={
 bitwise:{duration:96,scenes:6,cuts:{12:['bw-bits','bw-mask'],30:['bw-bits','bw-mask','bw-work'],48:['bw-bits','bw-work'],68:['bw-bits','bw-work'],86:['bw-bits','bw-work','bw-left-saved']},cues:{ready:8,and:24,or:38,xor:46,not:56,promotion:65,left:77,right:85,'shift-guard':93},
  early:[[0,'bw-bits'],[12,'bw-work'],[58.9,'bw-small'],[75.6,'bw-left-saved']],actions:[14.3,50.4,70.7,78.7]},
 integers:{duration:128,scenes:8,cuts:{12:['int-pattern','int-u8','int-s8'],30:['int-pattern','int-u8','int-s8'],46:['int-pattern','int-u8','int-s8'],80:['int-wide','int-truncated'],98:['int-wide','int-truncated','int-wrap-source','int-wrapped'],116:['int-wide','int-truncated','int-wrap-source','int-wrapped','int-wide-sum','int-promoted']},cues:{ready:8,representation:26,invert8:37,negate8:42,extend:59,truncate:76,'unsigned-wrap':94,'signed-guard':109,'promoted-add':114},
  early:[[0,'int-pattern'],[32.9,'int-magnitude'],[47.9,'int-zero'],[64.9,'int-wide'],[81.9,'int-wrap-source'],[100.4,'int-signed-lhs'],[108.2,'int-rejected']],actions:[15.3,34.7,38.7,50.7,56,69.4,87.5,104.7]}
};
const browser=await chromium.launch();await mkdir(resolve(root,'film-artifacts'),{recursive:true});
try{
 for(const published of process.env.IBIT_SOURCE_ONLY?[false]:[false,true]){
  const server=createPreviewServer(published?{directory:resolve(root,'dist'),prefix:'/csapp'}:{});await new Promise(r=>server.listen(0,'127.0.0.1',r));
  const origin=`http://127.0.0.1:${server.address().port}`,base=origin+(published?'/csapp':'');
  try{
   for(const[id,plan]of Object.entries(plans)){
    const p=await browser.newPage({viewport:{width:1600,height:1160}});
    p.on('pageerror',e=>errors.push([id,e.message]));p.on('response',r=>{if(r.status()>=400)errors.push([id,r.status(),r.url()]);});p.on('request',r=>{if(!r.url().startsWith(origin)&&!r.url().startsWith('data:'))errors.push(['remote',r.url()]);});p.on('console',m=>{if(m.type()==='error')errors.push(['console',m.text()]);});
    await p.goto(`${base}/chapter2/${id}.html?paused`);await p.waitForFunction(()=>window.film);
    eq(await p.evaluate(()=>[film.duration,film.chapters.length,film.dataSource]),[plan.duration,plan.scenes,'gdb-plus-model'],'independent film, evidence and scope');
    eq((await p.locator('#experiment-source > span code').allTextContents()).join('\n'),raw.provenance.source.content,'complete unchanged actual C');
    await p.evaluate(()=>{const r=document.querySelector('[data-film-continuity]');window.originalBits=[r,...r.querySelectorAll('*')];window.bitElementReplacements=[];new MutationObserver(rs=>{for(const r of rs)if([...r.addedNodes,...r.removedNodes].some(n=>n.nodeType===1))bitElementReplacements.push(r.target.id||r.target.className);}).observe(r,{childList:true,subtree:true});});
    for(const[time,node]of plan.early){await seek(p,time);eq(await shown(p,node),false,'future object hidden '+node);}
    const typography=await p.evaluate(()=>[...document.querySelectorAll('.ibit-name,.ibit-ribbon-name,.ibit-type,.ibit-short,.bit-value')].every(n=>{const s=getComputedStyle(n);return parseFloat(s.fontSize)>=20&&s.color==='rgb(255, 255, 255)';}));ok(typography,'readable ordinary white labels and bits');
    const forward=[];
    for(const[cut,ids]of Object.entries(plan.cuts)){
     const b=Number(cut),states=[];
     for(const delta of [-.001,.001]){await seek(p,b+delta);states.push(await p.evaluate(ids=>ids.map(id=>{const e=document.getElementById(id),r=e.getBoundingClientRect();let visible=true;for(let n=e;n&&n.id!=='film-stage';n=n.parentElement){const s=getComputedStyle(n);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)<.999)visible=false;}return{visible,x:r.x,y:r.y,text:[...e.querySelectorAll('.bit-value,.ibit-full,.ibit-ribbon-hex,.reg-value')].map(n=>n.textContent)};}),ids));}
     ids.forEach((node,j)=>{ok(states[0][j].visible&&states[1][j].visible,'source/result stays visible at cut '+node);eq(states[0][j].text,states[1][j].text,'same required data across cut '+node);ok(Math.hypot(states[0][j].x-states[1][j].x,states[0][j].y-states[1][j].y)<2,'continuous coordinates '+node);});
    }
    // Numerical expectations come only from the independently read raw trace.
    if(id==='bitwise'){
     for(const[event,time]of Object.entries(plan.cues)){await seek(p,time);await assertWord(p,'bw-bits',ss[0].fields.bits);if(['and','or','xor','not','left','right'].includes(event))await assertWord(p,'bw-work',v(event,'r_'+event));}
     for(const[before,commit]of [[21.99,22],[37.19,37.2],[45.19,45.2],[53.99,54],[75.59,75.6],[83.59,83.6]]){await seek(p,before);eq(await p.locator('#bw-work .ibit-full').textContent(),'推导中','no future C full result before observation');await seek(p,commit+.01);ok((await p.locator('#bw-work .ibit-full').textContent()).startsWith('0x'),'result atomically committed');}
     await seek(p,65);eq(await p.locator('#bw-promoted .reg-value').textContent(),String(v('promotion','promoted_not')),'actual int32 NOT after promotion');eq(await p.locator('#bw-narrow .reg-value').textContent(),hex(v('promotion','narrow_not'),8),'explicit uint8 narrowing');
     await seek(p,95);ok((await p.locator('#bw-summary-note').textContent()).includes('C11负数右移为实现定义'),'no portable signed negative shift claim');ok((await p.locator('#bw-blocked').textContent()).includes('未执行非法表达式'),'invalid shift not executed');
    }else{
     for(const time of [8,26,42,59]){await seek(p,time);await assertRibbon(p,'int-pattern',ss[0].fields.u8,8);eq(await p.locator('#int-u8 .reg-value').textContent(),String(ss[0].fields.u8),'same unsigned object');eq(await p.locator('#int-s8 .reg-value').textContent(),String(ss[0].fields.s8),'same signed object');}
     await seek(p,26);eq(await p.locator('#int-weight-0').textContent(),'-128','highest bit is a negative weight, not independent sign');
     for(const[time,event,key]of [[37,'invert8','inverted8'],[42,'negate8','negated8']]){await seek(p,time);await assertRibbon(p,'int-neg-work',v(event,key),8);}
     await seek(p,59);await assertRibbon(p,'int-zero',v('extend','zero_extended'),16);await assertRibbon(p,'int-sign',v('extend','sign_extended'),16);
     for(const time of [76,94,114,127]){await seek(p,time);await assertRibbon(p,'int-wide',ss[0].fields.wide_unsigned,32);await assertRibbon(p,'int-truncated',v('truncate','truncated'),8);}
     for(const time of [94,114,127]){await seek(p,time);await assertRibbon(p,'int-wrap-source',ss[0].fields.wrap_lhs,32);await assertRibbon(p,'int-wrapped',v('unsigned-wrap','wrapped'),32);}
     await seek(p,114);eq(await p.locator('#int-wide-sum .reg-value').textContent(),String(v('signed-guard','wide_sum')),'actual int64 result, not wrapped int32');eq(await p.locator('#int-promoted .reg-value').textContent(),String(v('promoted-add','promoted_add')),'int8 addition really promotes to int128');ok((await p.locator('#int-rejected').textContent()).includes('未执行'),'signed overflow rejected');
     for(const[time,node]of [[57.49,'int-sign'],[73.99,'int-truncated'],[91.99,'int-wrapped']]){await seek(p,time);eq(await p.locator('#'+node+' .ibit-ribbon-hex').textContent(),'待推导','no early final C encoding');}
    }
    // Stable data, text and geometry must also survive backwards / random seeking.
    const times=[...Object.values(plan.cues),plan.duration-1,0,plan.duration*.13,plan.duration*.77];
    const signature=()=>p.evaluate(()=>[...document.querySelectorAll('[data-film-continuity] :is(.ibit-word,.ibit-ribbon,.register-cell)')].map(e=>{const r=e.getBoundingClientRect(),s=getComputedStyle(e);return[e.id,e.textContent,s.opacity,s.visibility,r.x.toFixed(2),r.y.toFixed(2),r.width.toFixed(2)];}));
    for(const time of times){await seek(p,time);forward.push(await signature());}
    for(let i=times.length-1;i>=0;i--){await seek(p,times[i]);eq(await signature(),forward[i],'reverse exact text / bits / layout');ok(await p.evaluate(()=>originalBits.every(n=>document.contains(n))),'same original elements, including every bit');}
    const movement=id==='bitwise'?[['bw-and-a-0',14.2,14.55],['bw-left-0',70.2,70.7]]:[['int-sign-high-0',55.4,55.7],['int-carry',86.4,88]];
    for(const[key,a,b]of movement){const positions=[];for(const time of[a,b]){await seek(p,time);ok(await shown(p,key+'-packet'),'packet is really visible during transfer');positions.push(await p.locator('#'+key+'-packet').evaluate(n=>{const r=n.getBoundingClientRect();return[r.x,r.y];}));}ok(Math.hypot(positions[0][0]-positions[1][0],positions[0][1]-positions[1][1])>12,'real path movement, not an SVG that stays in place');}
    const sampleTimes=[...new Set([...Array.from({length:Math.ceil(plan.duration/.4)},(_,i)=>.08+i*.4),...Object.keys(plan.cuts).flatMap(t=>[-.4,0,.18,.35,.5,.7,.95,1.25,1.6,2.2].map(d=>Number(t)+d)),...plan.actions.flatMap(t=>[-.15,0,.2,.4,.6,.85].map(d=>t+d))])].filter(t=>t>=0&&t<plan.duration);
    for(const time of sampleTimes){await seek(p,time);const result=await reviewVisibility(p);if(Object.values(result).some(a=>a.length))issues.push({variant:published?'published':'source',id,time,result});checks++;}
    for(const[cut,ids]of Object.entries(plan.cuts)){
     const boundary=Number(cut);await seek(p,boundary-.22);
     const frames=await p.evaluate(async({boundary,ids})=>{const saved=ids.map(id=>document.getElementById(id)),rows=[];film.play();await new Promise(resolve=>{function tick(){const time=film.getState().time;rows.push({time,valid:ids.every((id,i)=>{let e=document.getElementById(id);if(e!==saved[i])return false;for(;e&&e.id!=='film-stage';e=e.parentElement){const s=getComputedStyle(e);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)!==1)return false;}return true;})});if(time>boundary+.28){film.pause();resolve();}else requestAnimationFrame(tick);}requestAnimationFrame(tick);});return rows;},{boundary,ids});
     ok(frames.length>4&&frames.some(r=>r.time<boundary)&&frames.some(r=>r.time>=boundary),'actual rAF on both sides of cut');ok(frames.every(r=>r.valid),'no fading/replacement during actual boundary playback');
    }
    const cues=await p.evaluate(()=>film.experimentCues);
    eq(cues.length,Object.keys(plan.cues).length,'only related observations have claimed film locations');
    for(const s of ss){await p.selectOption('#experiment-stop',String(s.index));eq(await p.locator('#experiment-command-output').textContent(),s.commands.map(c=>'(gdb) '+c.command+'\n'+c.output).join('\n\n'),'all original GDB commands');
     for(const[name,o]of Object.entries(s.objects)){const row=await p.locator(`tr[data-field="${name}"]`).innerText();ok(row.includes(String(s.fields[name]))&&row.includes(o.address)&&row.includes(o.type+' / '+o.width)&&row.includes(o.bytes.map(b=>b.toString(16).toUpperCase().padStart(2,'0')).join(' ')),'40 actual typed objects per stop '+name);}
     const cue=cues.find(c=>c.sampleIndex===s.index);await p.locator('#experiment-locate').click();eq(await p.evaluate(()=>film.getState().time),cue?plan.cues[s.event]:0,'honest relevant or unsupported location');if(!cue)ok((await p.locator('#experiment-locate').textContent()).includes('保留完整原始记录'),'unsupported observations not labeled exact');
    }
    eq(await p.locator('#integer-expression-types').textContent(),raw.types.map(t=>`(gdb) ${t.evidence.command}\n${t.evidence.output}\n实际宽度=${t.width}，${t.signed?'signed':'unsigned'}`).join('\n\n'),'actual raw GDB promotion evidence');
    for(const[path,hash]of [[raw.provenance.source.path,raw.provenance.source.sha256],[raw.provenance.binary.path,raw.provenance.binary.sha256]]){const r=await p.request.get(base+'/'+path);eq(r.status(),200,'download source/ELF');eq(sha(await r.body()),hash,'independent evidence hash');}
    for(const a of await p.locator('.experiment-links a').all())eq((await p.request.get(new URL(await a.getAttribute('href'),p.url()).href)).status(),200,'offline local evidence/document');
    eq(await p.evaluate(()=>bitElementReplacements),[],'no cloned replacement elements in original actors');
    if(!published){for(const[i,time]of plan.actions.entries()){await seek(p,time);await p.locator('.film-viewport').screenshot({path:resolve(root,'film-artifacts',`${id}-action-${i+1}.png`)});}await p.locator('.integer-observations').scrollIntoViewIfNeeded();await p.screenshot({path:resolve(root,'film-artifacts',`${id}-lab.png`)});}
    await p.setViewportSize({width:390,height:850});ok(await p.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'390px lab / source / controls no horizontal page overflow');
    films.push({id,variant:published?'published':'source',sampledTimes:sampleTimes.length,cuts:Object.keys(plan.cuts).length});await p.close();
   }
   const guide=await browser.newPage({viewport:{width:390,height:850}});await guide.goto(base+'/chapter2/guide.html');ok(await guide.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'guide reflows at390');ok((await guide.locator('article').innerText()).includes('本程序没有执行溢出的int32加法'),'guide explains no UB result');for(const a of await guide.locator('article a').all())eq((await guide.request.get(new URL(await a.getAttribute('href'),guide.url()).href)).status(),200,'guide links');await guide.close();
  }finally{await new Promise(r=>server.close(r));}
 }
 eq(errors,[],'no script/SVG errors, missing resources or remote runtime');
 await writeFile(resolve(root,'film-artifacts/integer-bits-verification.json'),JSON.stringify({checks:checks+1,films,issues,errors,stops:ss.length},null,2));
 if(!process.env.IBIT_DIAGNOSTIC)eq(issues,[],'shared/local text, moving packets and opaque panels readable; see integer-bits-verification.json');else console.log('DIAGNOSTIC issues',issues.length,JSON.stringify(issues.slice(0,8)));
 console.log(`${process.env.IBIT_DIAGNOSTIC?'DIAGNOSTIC':'PASS'} ${checks} integer/bitwise data, type, state, identity, continuity, C/GDB, lab and motion checks`);
}finally{await browser.close();}
