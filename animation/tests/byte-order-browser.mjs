import assert from 'node:assert/strict';
import {readFile,writeFile,mkdir} from 'node:fs/promises';
import {createHash} from 'node:crypto';
import {resolve} from 'node:path';
import {chromium} from 'playwright';
import {createPreviewServer} from '../preview-server.mjs';
const root=resolve(import.meta.dirname,'..'), raw=JSON.parse(await readFile(resolve(root,'traces/byte-order.json'),'utf8'));
const samples=raw.runs.single.samples, first=samples[0], be=samples[8].objects.be_encoded.bytes;
const byte=n=>n.toString(16).toUpperCase().padStart(2,'0'), word=n=>'0x'+n.toString(16).toUpperCase().padStart(8,'0');
const commit=[51.08,53.08,55.08,57.08];
let checks=0;const errors=[];
const eq=(a,b,message)=>{assert.deepEqual(a,b,message);checks++;},ok=(v,m)=>{assert.ok(v,m);checks++;};
const seek=(p,time)=>p.evaluate(t=>film.seek(t),time);
// Works on the actual visible text and all opaque teaching panels, including
// the cross-scene layer; future/hidden scene text is not counted.
async function visibilityReview(page,packets=true){
  return page.evaluate(packets=>{
    const host=document.querySelector('#scene-host'), stage=document.querySelector('#film-stage').getBoundingClientRect(), scale=stage.width/1440;
    const visible=el=>{for(let n=el;n&&n!==host;n=n.parentElement){const s=getComputedStyle(n);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)<.5)return false;}return !!el.getClientRects().length;};
    const glyphs=[],walker=document.createTreeWalker(host,NodeFilter.SHOW_TEXT);
    while(walker.nextNode()){
      const n=walker.currentNode;if(!n.textContent.trim()||n.parentElement.closest('svg')||!visible(n.parentElement))continue;
      const range=document.createRange();range.selectNodeContents(n);
      for(const r of range.getClientRects())if(r.width&&r.height)glyphs.push({text:n.textContent,el:n.parentElement,r});
    }
    const safe=glyphs.filter(({r})=>(r.left-stage.left)/scale<15||(r.right-stage.left)/scale>1425||(r.top-stage.top)/scale<72||(r.bottom-stage.top)/scale>770).map(g=>g.text);
    const intersects=(a,b)=>a.left<b.right-1&&a.right>b.left+1&&a.top<b.bottom-1&&a.bottom>b.top+1;
    const moving=packets?[...host.querySelectorAll('.moving-packet')].filter(visible):[];
    const motion=moving.flatMap(p=>glyphs.filter(g=>intersects(p.getBoundingClientRect(),g.r)).map(g=>[p.id,g.text]));
    const panels=[...host.querySelectorAll('.register-cell,.vertical-array,.code-snippet')].filter(visible);
    const opaque=glyphs.flatMap(g=>panels.filter(p=>!p.contains(g.el)&&intersects(p.getBoundingClientRect(),g.r)).map(p=>[p.id,g.text]));
    const text=[];for(let i=0;i<glyphs.length;i++)for(let j=i+1;j<glyphs.length;j++){
      const a=glyphs[i],b=glyphs[j],w=Math.min(a.r.right,b.r.right)-Math.max(a.r.left,b.r.left),h=Math.min(a.r.bottom,b.r.bottom)-Math.max(a.r.top,b.r.top);
      if(w>3&&h>6&&w*h>50)text.push([a.text,b.text]);
    }
    return {safe,motion,opaque,text};
  },packets);
}
const browser=await chromium.launch();
await mkdir(resolve(root,'film-artifacts'),{recursive:true});
try{
 for(const published of [false,true]){
  const server=createPreviewServer(published?{directory:resolve(root,'dist'),prefix:'/csapp'}:{});
  await new Promise(r=>server.listen(0,'127.0.0.1',r));
  const origin=`http://127.0.0.1:${server.address().port}`,base=origin+(published?'/csapp':'');
  try{
   const page=await browser.newPage({viewport:{width:1440,height:1100}});
   page.on('pageerror',e=>errors.push(e.message));page.on('response',r=>{if(r.status()>=400)errors.push(r.status()+' '+r.url());});
   page.on('request',r=>{if(!r.url().startsWith(origin)&&!r.url().startsWith('data:'))errors.push('Remote '+r.url());});
   await page.goto(base+'/chapter2/byte-order.html?paused');await page.waitForFunction(()=>window.film);
   eq(await page.evaluate(()=>[film.duration,film.chapters.length,film.dataSource,film.continuity.rootId]),[96,6,'gdb-measured','byte-order-retained'],'duration, six local compositions and same-object layer');
   eq((await page.locator('#experiment-source > span code').allTextContents()).join('\n'),raw.provenance.source.content,'unmodified complete C');
   await page.evaluate(()=>{window.retainedTest=Object.fromEntries(['bo-word','bo-address','bo-native','bo-be',...[0,1,2,3].flatMap(i=>['bo-native-'+i,'bo-be-'+i])].map(id=>[id,document.getElementById(id)]));});
   for(const [time,selector,visible] of [[0,'#bo-word',false],[11,'#bo-native',false],[25,'#bo-native',false],[43,'#bo-be',false],[48.5,'#bo-be',false],[61,'#bo-be',true],[68,'#bo-raw-read',false],[74,'#bo-decoded',false]]){
     await seek(page,time);eq(await page.locator(selector).isVisible(),visible,'future content lifetime at '+time);
   }
   const boundaries=[12,26,44,62,82],times=boundaries.flatMap(t=>[-.6,-.01,0,.01,.2,.6,1.3].map(d=>t+d));
   for(const time of [...times,...times.toReversed(),95,1.8,70,33,58,25,85,50]){
     await seek(page,time);
     const state=await page.evaluate(()=>{
       const visible=el=>{for(let n=el;n&&n.id!=='scene-host';n=n.parentElement){const s=getComputedStyle(n);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)<.999)return false;}return true;};
       return {identity:Object.entries(retainedTest).every(([id,node])=>node===document.getElementById(id)),
         shown:Object.fromEntries(Object.entries(retainedTest).map(([id,node])=>[id,visible(node)])),
         local:document.querySelectorAll('.scene:not([hidden])').length};
     });
     ok(state.identity,'same DOM identities in forward/reverse/arbitrary seek '+time);
     eq(state.local,1,'only current local annotation scene');
     for(const id of ['bo-word','bo-address',...(time>28.1?['bo-native']:[]),...(time>49.9?['bo-be']:[])])ok(state.shown[id],'no fade or clearing of needed '+id+' at '+time);
     eq(await page.locator('#bo-word .reg-value').textContent(),word(first.fields.word),'source value preserved');
     eq(await page.locator('#bo-address .reg-value').textContent(),first.objects.word.address,'same execution / same full address');
     for(let i=0;i<4;i++){
       eq(await page.locator(`#bo-native-${i} .array-value`).textContent(),byte(first.objects.word.bytes[i]),'native word never rewritten');
       eq(await page.locator(`#bo-be-${i} .array-value`).textContent(),byte(time>=commit[i]?be[i]:first.objects.be_encoded.bytes[i]),'exact fixed-boundary BE commit');
     }
   }
   for(const boundary of boundaries){
     const ids=['bo-word','bo-address',...(boundary>=44?['bo-native']:[]),...(boundary>=62?['bo-be']:[])];
     const positions=[];
     for(const delta of [-.001,0,.001]){await seek(page,boundary+delta);positions.push(await page.evaluate(ids=>ids.map(id=>{const r=document.getElementById(id).getBoundingClientRect();return[r.x,r.y];}),ids));}
     for(let i=0;i<ids.length;i++)for(let j=1;j<3;j++)ok(Math.hypot(positions[j][i][0]-positions[j-1][i][0],positions[j][i][1]-positions[j-1][i][1])<2,'continuous position, not a rebuilt next frame '+ids[i]);
   }
   // Check the program's known bytes and observations, independent of renderer models.
   const cues=await page.evaluate(()=>film.experimentCues);
   eq(cues.length,samples.length,'every actual observation has a truthful location');
   for(const s of samples){
     await page.selectOption('#experiment-stop',String(s.index));
     eq(await page.locator('#experiment-command-output').textContent(),s.commands.map(c=>'(gdb) '+c.command+'\n'+c.output).join('\n\n'),'original GDB commands');
     const obs=await page.locator('#byte-order-observation').textContent();
     ok(obs.includes(s.objects.word.address)&&obs.includes(s.objects.be_encoded.bytes.map(byte).join(' ')),'raw selected addresses/bytes');
     await page.locator('#experiment-locate').click();
     eq(await page.evaluate(()=>film.getState().time),cues[s.index].time,'exact C observation location');
     if(s.phase===2)eq(await page.locator('#bo-probe .reg-value').textContent(),byte(s.fields.probe_byte),'actual read committed at location');
     if(s.phase===3)for(let i=0;i<4;i++)eq(await page.locator(`#bo-be-${i} .array-value`).textContent(),byte(s.objects.be_encoded.bytes[i]),'actual byte commit at location');
   }
   for(const [time,id,field] of [[66,'bo-native-read','native_read'],[72,'bo-raw-read','raw_read'],[78,'bo-decoded','decoded_be']]){
     await seek(page,time);eq(await page.locator('#'+id+' .reg-value').textContent(),word(samples.at(-1).fields[field]),'real interpretation result');
   }
   await seek(page,61);
   for(const id of ['bo-native','bo-be']){
     eq(await page.locator('#'+id+' .array-row').count(),4,'fixed four-byte capacity');
     const data=await page.locator('#'+id+' .array-row').evaluateAll(rows=>rows.map(r=>({address:r.dataset.address,y:r.getBoundingClientRect().y,text:r.querySelector('.array-address').textContent})));
     for(let i=1;i<4;i++){ok(BigInt(data[i-1].address)>BigInt(data[i].address)&&data[i-1].y<data[i].y,'high VA up, low VA down');eq(data[i].text,data[i].address,'complete address');}
   }
   const movementTimes=[...boundaries.flatMap(t=>[-.3,0,.2,.6,1,1.6].map(d=>t+d)),12.8,13.2,13.6,26.4,26.8,44.8,45.4,45.8,46.4,47.8,48.5,49.2,49.8,
     ...[29,32,35,38,50,52,54,56,64,70,76].flatMap(t=>[.12,.35,.6,.85,1.04].map(d=>t+d))];
   for(const time of movementTimes){await seek(page,time);eq(await visibilityReview(page),{safe:[],motion:[],opaque:[],text:[]},'moving/transition glyph and panel visibility at '+time);}
   for(const [path,sha] of [[raw.provenance.source.path,raw.provenance.source.sha256],[raw.provenance.binary.path,raw.provenance.binary.sha256]]){
     const response=await page.request.get(base+'/'+path);eq(response.status(),200,'evidence available');eq(createHash('sha256').update(await response.body()).digest('hex'),sha,'independent download hash');
   }
   for(const a of await page.locator('.experiment-links a').all())eq((await page.request.get(new URL(await a.getAttribute('href'),page.url()).href)).status(),200,'local evidence links');
   // Real playback through each cut; inspect every animation frame, not only seeks.
   for(const boundary of boundaries){
     await seek(page,boundary-.25);
     const recorded=await page.evaluate(async boundary=>{
       const rows=[];film.play();
       await new Promise(resolve=>{function sample(){const t=film.getState().time;const ids=['bo-word','bo-address',...(boundary>=44?['bo-native']:[]),...(boundary>=62?['bo-be']:[])];
         rows.push({time:t,valid:ids.every(id=>{const n=document.getElementById(id);if(n!==retainedTest[id])return false;for(let el=n;el&&el.id!=='film-stage';el=el.parentElement){const s=getComputedStyle(el);if(s.display==='none'||s.visibility!=='visible'||Number(s.opacity)!==1||el.hidden)return false;}return true;})});
         if(t>=boundary+.3){film.pause();resolve();}else requestAnimationFrame(sample);}requestAnimationFrame(sample);});return rows;
     },boundary);
     ok(recorded.length>5&&recorded.some(r=>r.time<boundary)&&recorded.some(r=>r.time>=boundary),'actual frames on both sides of cut');
     ok(recorded.every(r=>r.valid),'no full-clear/fade in actual boundary playback');
   }
   await seek(page,50.7);await page.locator('.film-viewport').screenshot({path:resolve(root,'film-artifacts',`byte-order-action${published?'-published':''}.png`)});
   await page.locator('#experiment-workbench').scrollIntoViewIfNeeded();await page.locator('#experiment-workbench').screenshot({path:resolve(root,'film-artifacts',`byte-order-lab${published?'-published':''}.png`)});
   await page.setViewportSize({width:390,height:850});ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'narrow source/workbench');
   await page.close();
  }finally{await new Promise(r=>server.close(r));}
 }
 eq(errors,[],'no missing resources, script errors or remote runtime');
 await writeFile(resolve(root,'film-artifacts/byte-order-verification.json'),JSON.stringify({checks,stops:samples.length,variants:['source','/csapp/'],errors},null,2));
 console.log(`PASS: ${checks} independent byte-order / retained-DOM / real-boundary / source / memory / motion checks`);
}finally{await browser.close();}
