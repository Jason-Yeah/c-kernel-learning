import assert from 'node:assert/strict';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {resolve} from 'node:path';
import {chromium} from 'playwright';
import {films} from '../shared/catalog.js';
import {createPreviewServer} from '../preview-server.mjs';
const root=resolve(import.meta.dirname,'..');
let checks=0;const errors=[],issues=[],summaries=[];
const eq=(a,b,m)=>{assert.deepEqual(a,b,m);checks++;},ok=(v,m)=>{assert.ok(v,m);checks++;};
const seek=(p,t)=>p.evaluate(t=>film.seek(t),t);
const expected={'os-compile':3,'os-elf':1,'os-loading':2,'os-process-space':1,'os-parent-child':2,'os-registers':2,'instruction-cycle':3,'stack-frame':3,'stack-overflow':3,'cache':6,'process-lifecycle':6,'fork-memory-layout':2,'address-translation':1};
async function actorState(p,id,kind,keys=null){return p.evaluate(({id,kind,keys})=>{
 const el=document.getElementById(id),stage=document.getElementById('film-stage').getBoundingClientRect(),r=el.getBoundingClientRect();
 let visible=true;for(let n=el;n&&n.id!=='film-stage';n=n.parentElement){const s=getComputedStyle(n);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)<.999)visible=false;}
 let data;if(kind==='array')data=Object.fromEntries([...el.querySelectorAll('.array-row')].filter(row=>!keys||keys.includes(row.dataset.memoryKey)).map(row=>[row.dataset.memoryKey,[row.querySelector('.array-address').textContent,row.querySelector('.array-value').textContent]]));
 else if(kind==='register')data=el.querySelector('.reg-value').textContent;
 else if(kind==='cache')data=[el.querySelector('.cache-meta b').textContent,...[...el.querySelectorAll('.cache-byte')].map(n=>n.textContent),el.querySelector('.cache-rank').textContent];
 else if(kind==='file')data=el.querySelector('strong').textContent;
 else if(kind==='code')data=[...el.querySelectorAll('.code-line code')].map(n=>n.textContent);
 else if(kind==='process')data=[el.querySelector('.process-pid').textContent,el.querySelector('.process-image').textContent];
 else if(kind==='page')data=el.querySelector('.page-value').textContent;
 else if(kind==='text')data=el.textContent;
 return {visible,data,x:(r.x-stage.x)*1440/stage.width,y:(r.y-stage.y)*1440/stage.width};
 },{id,kind,keys});}
async function review(p){return p.evaluate(()=>{
 const host=document.querySelector('#scene-host'),stage=document.getElementById('film-stage').getBoundingClientRect(),scale=stage.width/1440;
 const visible=el=>{for(let n=el;n&&n!==host;n=n.parentElement){const s=getComputedStyle(n);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)<.5)return false;}return !!el.getClientRects().length;};
 const texts=[],walker=document.createTreeWalker(host,NodeFilter.SHOW_TEXT);
 while(walker.nextNode()){
   const node=walker.currentNode;if(!node.textContent.trim()||node.parentElement.closest('svg')||!visible(node.parentElement))continue;
   const range=document.createRange();range.selectNodeContents(node);
   for(const r of range.getClientRects())if(r.width>1&&r.height>1)texts.push({el:node.parentElement,text:node.textContent.trim(),r});
 }
 const bounds=texts.filter(({r})=>(r.x-stage.x)/scale<15||(r.right-stage.x)/scale>1425||(r.y-stage.y)/scale<72||(r.bottom-stage.y)/scale>770).map(g=>g.text);
 const intersect=(a,b)=>Math.min(a.right,b.right)-Math.max(a.left,b.left)>2&&Math.min(a.bottom,b.bottom)-Math.max(a.top,b.top)>3;
 const collisions=[];for(let i=0;i<texts.length;i++)for(let j=i+1;j<texts.length;j++){
   const a=texts[i].r,b=texts[j].r,w=Math.min(a.right,b.right)-Math.max(a.left,b.left),h=Math.min(a.bottom,b.bottom)-Math.max(a.top,b.top);
   if(w>3&&h>6&&w*h>50)collisions.push([texts[i].text,texts[j].text]);
 }
 const packets=[...host.querySelectorAll('.moving-packet,.packet')].filter(visible);
 const motion=packets.flatMap(el=>texts.filter(t=>intersect(el.getBoundingClientRect(),t.r)).map(t=>[el.id,t.text]));
 const occluded=[];
 for(const g of texts){
   const hit=document.elementFromPoint(g.r.left+Math.min(g.r.width/2,8),g.r.top+g.r.height/2);
   if(hit&&!g.el.contains(hit)&&hit.closest('.register-cell,.vertical-array,.code-snippet,.program-window,.cache-way,.memory-page,.os-file,.task-record'))occluded.push([g.text,hit.closest('[id]')?.id]);
 }
 return {bounds,collisions,motion,occluded};
});}
const rawCache=JSON.parse(await readFile(resolve(root,'traces/cache-access.json'),'utf8'));
const cacheSamples=Object.values(rawCache.runs)[0].samples, bytes=cacheSamples[0].fields.memory;
const cache=Array.from({length:4},()=>[0,1].map(()=>({valid:false,tag:0,data:[],used:0,block:0})));
let hits=0,misses=0;const records=[];
for(const [i,s]of cacheSamples.filter(s=>s.phase===2).entries()){
 const address=s.fields.offset,set=Math.floor(address/8)%4,tag=Math.floor(address/32),before=structuredClone(cache[set]);
 let way=cache[set].findIndex(l=>l.valid&&l.tag===tag);const hit=way>=0;
 if(hit)hits++;else{misses++;way=cache[set].findIndex(l=>!l.valid);if(way<0)way=cache[set][0].used<cache[set][1].used?0:1;const block=Math.floor(address/8)*8;cache[set][way]={valid:true,tag,data:bytes.slice(block,block+8),used:0,block};}
 cache[set][way].used=i+1;
 records.push({set,before,after:structuredClone(cache[set]),hit,value:s.fields.value,hits,misses});
}
const hex=n=>n.toString(16).toUpperCase().padStart(2,'0');
async function existingRecursion(p,base) {
 await p.goto(base+'/chapter3/stack-exhaustion-film.html?paused');await p.waitForFunction(()=>window.film);
 eq(await p.evaluate(()=>film.duration-film.experimentDuration),30,'approved recursion mechanism remains 30 seconds');
 eq(await p.evaluate(()=>film.carriedActors.length),0,'do not replace the approved camera matching merely to unify architecture');
 for(const [time,before,after]of [[16.5,'call-frame','recurse-frame-0'],[22.5,'recurse-frame-0','capacity-frame-0'],[28.5,'capacity-frame-7','overflow-valid']]){
  const rect=async id=>p.locator('#'+id).evaluate(el=>{const r=el.getBoundingClientRect(),s=getComputedStyle(el);return{x:r.x,y:r.y,w:r.width,h:r.height,visible:s.visibility==='visible'&&Number(s.opacity)===1};});
  await seek(p,time-.001);const a=await rect(before);await seek(p,time+.001);const b=await rect(after);
  ok(a.visible&&b.visible&&Math.hypot(a.x-b.x,a.y-b.y)<2&&Math.abs(a.w-b.w)<2&&Math.abs(a.h-b.h)<2,'legacy recursion already has a nonblank matched composition');
 }
 await seek(p,34.4);ok((await p.locator('#overflow-rewind').textContent()).includes('另一条正常'),'fault to bounded return remains an explicit independent comparison');
}
async function rejectedManifests(p,origin) {
 await p.goto(origin+'/');
 const messages=await p.evaluate(async()=>{
  const {prepareActorContinuity,actor}=await import('/shared/actor-continuity.js');
  const process=pid=>`<div class="window-title"><span>process</span></div><div class="process-pid">${pid}</div><div class="process-state">run</div><div class="process-image">same</div>`;
  const reg=name=>`<span class="reg-label"><strong class="reg-name">${name}</strong></span><b class="reg-value">5</b>`;
  const array=address=>`<div class="array-header"><strong>memory</strong><span>VA</span></div><div id="row-${address}" class="array-row" data-memory-key="object"><span class="array-name">object</span><code class="array-address">${address}</code><b class="array-value">5</b></div>`;
  const cases=[['process',process(1),process(2)],['register',reg('%rsp'),reg('%rbp')],['file','<strong>a.o</strong><span>file</span>','<strong>b.o</strong><span>file</span>'],['array',array('0x100'),array('0x200')]];
  return cases.map(([kind,a,b])=>{
    document.body.innerHTML=`<div id="scene-host"><section id="a"><div id="first" class="${kind==='array'?'vertical-array':''}">${a}</div></section><section id="b"><div id="next" class="${kind==='array'?'vertical-array':''}">${b}</div></section></div>`;
    const c=prepareActorContinuity([actor('invalid',kind,[['a','#first'],['b','#next']])]);
    try{c.bind('a',document.getElementById('a'));c.bind('b',document.getElementById('b'));return null;}catch(e){return e.message;}
  });
 });
 for(const message of messages)ok(message?.startsWith('Actor continuity:'),'different PIDs, files, physical registers and object VAs cannot be merged');
}
const browser=await chromium.launch();await mkdir(resolve(root,'film-artifacts'),{recursive:true});
try{
 for(const published of [false,true]){
  const server=createPreviewServer(published?{directory:resolve(root,'dist'),prefix:'/csapp'}:{});
  await new Promise(r=>server.listen(0,'127.0.0.1',r));const origin=`http://127.0.0.1:${server.address().port}`,base=origin+(published?'/csapp':'');
  try{
   for(const item of films.filter(f=>expected[f.id])){
    const p=await browser.newPage({viewport:{width:1600,height:1160}});
    p.on('pageerror',e=>errors.push([item.id,e.message]));p.on('response',r=>{if(r.status()>=400)errors.push([item.id,r.status(),r.url()]);});
    p.on('request',r=>{if(!r.url().startsWith(origin)&&!r.url().startsWith('data:'))errors.push(['remote',r.url()]);});
    await p.goto(`${base}/chapter${item.chapter}/${item.file}.html?paused`);await p.waitForFunction(()=>window.film);
    const meta=await p.evaluate(()=>({actors:film.carriedActors,chapters:film.chapters,duration:film.duration}));
    eq(meta.actors.length,expected[item.id],item.id+' original components, not a new corner badge');eq(meta.duration,item.duration,'original duration unchanged');
    await p.evaluate(()=>{
      window.originalActors=film.carriedActors.map(a=>({root:document.getElementById(a.rootId),elements:[...document.getElementById(a.rootId).querySelectorAll('*')]}));
      window.replacedActors=[];
      for(const a of originalActors)new MutationObserver(records=>{for(const r of records)if([...r.addedNodes,...r.removedNodes].some(n=>n.nodeType===1))replacedActors.push(r.target.id||r.target.className);}).observe(a.root,{subtree:true,childList:true});
    });
    const cuts=new Map();
    for(const a of meta.actors){
     const first=a.events[0];if(first.time>0){await seek(p,Math.max(0,first.time-.01));eq((await actorState(p,a.rootId,a.kind)).visible,false,'future actor hidden');}
     for(let i=1;i<a.events.length;i++){
      const e=a.events[i],prev=a.events[i-1];if(prev.scene===e.scene)continue;
      const common=e.keys&&prev.keys?e.keys.filter(k=>prev.keys.includes(k)):null;
      await seek(p,e.time-.001);const before=await actorState(p,a.rootId,a.kind,common);
      await seek(p,e.time+.001);const after=await actorState(p,a.rootId,a.kind,common);
      ok(before.visible&&after.visible,item.id+'/'+a.key+' remains fully visible across '+e.time);
      ok(Math.hypot(before.x-after.x,before.y-after.y)<2,item.id+'/'+a.key+' moves continuously, never teleports');
      if(!['mmu','chip'].includes(a.kind))eq(after.data,before.data,item.id+'/'+a.key+' retains required bytes/values/PID');
      const list=cuts.get(e.time)||[];list.push(a.rootId);cuts.set(e.time,list);
     }
    }
    const points=[...new Set([
      ...[...cuts.keys()].flatMap(t=>[-.35,-.01,0,.12,.3,.55,.8,1.1,1.4].map(d=>t+d)),
      ...Array.from({length:Math.floor(meta.duration/.65)},(_,i)=>.4+i*.65),
    ])].filter(t=>t>=0&&t<meta.duration);
    for(const time of [...points,...points.toReversed().filter((_,i)=>i%4===0)]){
      await seek(p,time);const result=await review(p);
      if(Object.values(result).some(v=>v.length))issues.push({variant:published?'published':'source',film:item.id,time,result});
      checks++;
    }
    for(const time of [meta.duration,0,meta.duration*.77,meta.duration*.11,...cuts.keys()]){
      await seek(p,time);ok(await p.evaluate(()=>originalActors.every(a=>document.contains(a.root)&&a.elements.every(e=>document.contains(e)))),'original element identities survive random/reverse seeks');
    }
    // Every cut is also actually played. rAF observes the same original nodes
    // through the boundary rather than inferring continuity from still images.
    for(const [time,ids]of cuts){
      await seek(p,time-.2);const frames=await p.evaluate(async({time,ids})=>{
        const saved=ids.map(id=>document.getElementById(id)), frames=[];film.play();
        await new Promise(resolve=>{function step(){const t=film.getState().time;frames.push({time:t,visible:ids.every((id,i)=>{let el=document.getElementById(id);if(el!==saved[i])return false;for(;el&&el.id!=='film-stage';el=el.parentElement){const s=getComputedStyle(el);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)!==1)return false;}return true;})});if(t>time+.25){film.pause();resolve();}else requestAnimationFrame(step);}requestAnimationFrame(step);});return frames;
      },{time,ids});
      ok(frames.length>4&&frames.some(r=>r.time<time)&&frames.some(r=>r.time>=time),'actual playback crosses both sides');ok(frames.every(r=>r.visible),'no shared-object fade in real playback');
    }
    if(item.id==='cache'){
      const durations=[10,6,8,6,9,9,8,6];
      for(const [i,r]of records.entries()){
        const chapter=meta.chapters.find(c=>c.id===`cache-access-${i}`),commit=r.hit?2.6:durations[i]-2.4;
        for(const [delta,which]of [[-.05,'before'],[.05,'after']]){
          await seek(p,chapter.start+commit+delta);
          for(const way of [0,1]){
            const line=r[which][way],id=`cache-${i}-before-${way}`;
            eq(await p.locator('#'+id+' .cache-meta b').textContent(),'TAG '+(line.valid?line.tag.toString(2).padStart(3,'0'):'---'),'independent raw-C-driven cache tags');
            eq(await p.locator('#'+id+' .cache-byte').allTextContents(),Array.from({length:8},(_,i)=>line.valid?hex(line.data[i]):'—'),'full cache line retained/committed');
          }
        }
        await seek(p,chapter.start+durations[i]-.9);
        eq(await p.locator(`#cache-${i}-AL .reg-value`).textContent(),hex(r.value),'actual C byte reaches same AL');
        eq(await p.locator(`#cache-${i}-counts .reg-value`).textContent(),`${r.hits} / ${r.misses}`,'independent hit history across cuts');
      }
    }
    await seek(p,meta.duration*.6);eq(await p.evaluate(()=>replacedActors),[],'no element replacement in live actors');
    summaries.push({id:item.id,variant:published?'published':'source',actors:meta.actors.length,cuts:cuts.size,sampledFrames:points.length});
    if(!published){
      const cut=[...cuts.keys()][0];await seek(p,cut+.55);await p.locator('.film-viewport').screenshot({path:resolve(root,'film-artifacts',`${item.id}-continuity-action.png`)});
    }
    await p.close();
   }
   const auditPage=await browser.newPage({viewport:{width:1600,height:1160}});
   await existingRecursion(auditPage,base);
   if(!published)await rejectedManifests(auditPage,origin);
   await auditPage.close();
  }finally{await new Promise(r=>server.close(r));}
 }
 eq(errors,[],'no errors/missing resources/CDN');
 await writeFile(resolve(root,'film-artifacts/continuity-verification.json'),JSON.stringify({checks:checks+1,films:summaries,issues,errors},null,2));
 eq(issues,[],'all shared/local glyphs, panels and moving packets remain readable (see continuity-verification.json)');
 console.log(`PASS ${checks} shared-node/state/byte/position/real-cut/occlusion checks; ${Object.keys(expected).length} earlier films, source and /csapp/`);
}finally{await browser.close();}
