// Adopt authored components, not screenshots/clones. Local timelines keep driving
// the actual shared children. Entry patches are inserted into those same local
// timelines so arbitrary forward/reverse seeks respect chronological state order.
import { gsap, number } from './scene-kit.js';
const fail = message => { throw new Error('Actor continuity: ' + message); };
const leaf = (root, selector) => root.querySelector(selector);
const text = (root, selector) => leaf(root, selector)?.textContent ?? '';
const placeholder = value => /^(?:—|…|\?|··)$/.test(value.trim());
const n = value => parseFloat(value) || 0;
function alias(node, id, inside = false) {
  if (!id || id === node.id) return;
  if (document.getElementById(id)) fail('duplicate alias ' + id);
  const wrapper = document.createElement('div');
  wrapper.id = id;
  wrapper.className = 'carry-alias';
  if (inside) {
    while (node.firstChild) wrapper.append(node.firstChild);
    node.append(wrapper);
  } else {
    node.before(wrapper);
    wrapper.append(node);
  }
}
function fields(root, kind) {
  const selectors = {
    page: ['.page-label', '.page-value'],
    chip: ['.chip-label', '.chip-subtitle'],
    file: ['strong', 'span'],
    process: ['.window-title > span', '.process-pid', '.process-state', '.process-image'],
    cache: ['.cache-meta span', '.cache-meta b', ...Array.from({length:8},(_,i)=>`.cache-byte[data-offset="${i}"]`), '.cache-rank'],
    mmu: ['.translation'],
    code: ['.code-heading', ...[...root.querySelectorAll('.code-line')].flatMap((_,i)=>[`.code-line[data-line="${i}"] i`,`.code-line[data-line="${i}"] code`])],
    text: ['@self'],
  };
  if (kind === 'register') {
    return leaf(root,'.reg-name') ? ['.reg-name','.reg-detail','.reg-value'] : ['.reg-label','.reg-value'];
  }
  return selectors[kind] || fail('unsupported component kind '+kind);
}
function regDetail(root) {
  if (leaf(root,'.reg-name') && !leaf(root,'.reg-detail')) {
    const span=document.createElement('span'); span.className='reg-detail'; leaf(root,'.reg-label').append(span);
  }
}
export const actor = (key, kind, members) => ({ key, kind, members: members.map(([scene, selector, options = {}]) => ({scene, selector, ...options})) });
export function prepareActorContinuity(spec = []) {
  const groups = spec.map(g => ({...g, entries:[], aliases:new Map(), rows:new Map()}));
  const aliases=new Map(), targets=new WeakMap();
  let current=null;
  function rewrite(selector) { return selector.replace(/#([\w-]+)/g,(all,id)=>'#'+(aliases.get(id)?.id || id)); }
  function bind(sceneId, root) {
    current={id:sceneId,groups:[]};
    for(const g of groups) for(const member of g.members.filter(m=>m.scene===sceneId)) {
      const incoming=root.querySelector(member.selector);
      if(!incoming)fail(`${g.key}: missing ${sceneId}/${member.selector}`);
      if(g.kind==='register')regDetail(incoming);
      const entry={...member, sceneRoot:root, incoming, left:n(incoming.style.left),top:n(incoming.style.top),width:incoming.style.width, snapshot:[]};
      if(!g.node) {
        g.node=incoming; g.base={left:entry.left,top:entry.top};
        g.firstAnimated=incoming.hasAttribute('data-anim');
        incoming.dataset.carryActor=g.key;
      }
      entry.first=g.entries.length===0;
      if(g.kind==='array') {
        if(!incoming.matches('.vertical-array'))fail(g.key+': expected keyed vertical array');
        entry.cell=n(incoming.style.getPropertyValue('--cell-height'));
        entry.keys=[...incoming.querySelectorAll('.array-row')].map(row=>row.dataset.memoryKey);
        entry.header=[text(incoming,'.array-header strong'),text(incoming,'.array-header span')];
        entry.rows=entry.keys.map(key=>{
          const row=incoming.querySelector(`.array-row[data-memory-key="${key}"]`);
          const state={key,id:row.id,label:text(row,'.array-name'),address:text(row,'.array-address'),value:text(row,'.array-value'),role:row.className,animated:row.hasAttribute('data-anim')};
          if(!g.rows.has(key)) {
            g.rows.set(key,row);
            if(!entry.first)g.node.append(row); // move a genuinely new object, not a retained-row clone
          }
          const original=g.rows.get(key);
          if(state.address !== text(original,'.array-address'))fail(g.key+': same row key with a different address');
          if(row!==original){row.remove();alias(original,state.id,true);}
          aliases.set(state.id,original); targets.set(original,{g,key});
          return state;
        });
      } else {
        if(g.kind==='file' && text(incoming,'strong')!==text(g.node,'strong'))fail(g.key+': different file identity');
        if(g.kind==='register') {
          const physical=name=>name.trim().toLowerCase().replace(/^%e(ax|sp|bp|ip)$/,'%r$1');
          if(physical(text(incoming,'.reg-name'))!==physical(text(g.node,'.reg-name')))fail(g.key+': different physical register');
        }
        const selectors=fields(incoming,g.kind);
        const originalSelectors=fields(g.node,g.kind);
        if(selectors.join('|')!==originalSelectors.join('|'))fail(g.key+': incompatible field structure');
        entry.snapshot=selectors.map(selector=>{
          const target=selector==='@self'?g.node:leaf(g.node,selector), source=selector==='@self'?incoming:leaf(incoming,selector);
          if(!target||!source)fail(g.key+': missing field '+selector);
          return {target,value:source.textContent};
        });
        if(g.kind==='process'&&text(incoming,'.process-pid')!==text(g.node,'.process-pid'))fail(g.key+': different process PID');
        if(g.kind==='mmu') {
          const child=leaf(incoming,'.mmu-tlb'), original=leaf(g.node,'.mmu-tlb');
          if(child!==original){const id=child.id;child.remove();alias(original,id);aliases.set(id,original);}
          entry.tlb=original;
        }
      }
      if(incoming!==g.node) {
        const id=incoming.id; incoming.remove();
        alias(g.node,id); aliases.set(id,g.node);
      } else aliases.set(incoming.id,g.node);
      targets.set(g.node,{g});
      g.entries.push(entry);current.groups.push({g,entry});
    }
    const adopted=current;
    return {
      query(selector,all=false) {
        const translated=rewrite(selector);
        const result=[...root.querySelectorAll(translated)];
        for(const {g,entry} of adopted.groups) {
          if(g.node.matches(translated))result.push(g.node);
          result.push(...g.node.querySelectorAll(translated));
        }
        const selected=[...new Set(result)].filter(el=>{
          const item=targets.get(el);
          if(item?.key && adopted.groups.some(x=>x.g===item.g)) {
            return adopted.groups.some(x=>x.g===item.g&&x.entry.keys?.includes(item.key));
          }
          return true;
        });
        if(selected.length && selected.every(el=>targets.get(el)?.key)) selected.sort((a,b)=>{
          const aa=targets.get(a),bb=targets.get(b);
          if(aa.g!==bb.g)return 0;
          const entry=adopted.groups.find(x=>x.g===aa.g).entry;
          return entry.keys.indexOf(aa.key)-entry.keys.indexOf(bb.key);
        });
        return all?selected:selected[0] || null;
      }
    };
  }
  function keepVisible(target) {
    if(!target || Array.isArray(target) || typeof target.length==='number')return false;
    const item=targets.get(target);if(!item||!current)return false;
    const entry=current.groups.find(x=>x.g===item.g)?.entry;
    if(!entry)return false;
    if(!item.key && !item.g.firstAnimated)return true;
    if(entry.first)return false;
    if(item.key) {
      const prev=item.g.entries[item.g.entries.indexOf(entry)-1];
      return prev.keys.includes(item.key);
    }
    return true;
  }
  function finish(chapters, duration, stretch) {
    current=null;
    if(!groups.length)return [];
    const host=document.createElement('section');host.className='film-continuity actor-continuity-host';host.dataset.filmContinuity='';
    host.setAttribute('aria-label','从上一镜头连续保留的对象');document.querySelector('#scene-host').prepend(host);
    const output=[];
    const value=(tl,target,v,at,first=false)=>{if(first||!placeholder(v))number(tl,target,v,at);};
    for(const g of groups) {
      if(g.entries.length<2)fail(g.key+': expected two or more authored views');
      const first=g.entries[0], last=g.entries.at(-1), owner=chapters.find(s=>s.root===first.sceneRoot);
      if(!owner)fail(g.key+': missing owner');
      const view=document.createElement('div');
      view.className='actor-continuity-view '+[...owner.root.classList].filter(c=>!['scene','directed-scene'].includes(c)).join(' ');
      // All outside aliases move with the one canonical node.
      let outer=g.node;while(outer.parentElement?.classList.contains('carry-alias'))outer=outer.parentElement;
      view.append(outer);host.append(view);
      g.node.setAttribute('data-anim',''); // future actors cannot leak before their first local timeline
      if(!g.firstAnimated) owner.timeline.set(g.node,{autoAlpha:1},first.at||0);
      if(g.kind==='array') {
        g.node.classList.add('carried-array');
        if(g.node.dataset.space==='virtual') {
          [...g.rows.values()].sort((a,b)=>{
            if(!/^0x/.test(a.dataset.address)||!/^0x/.test(b.dataset.address))fail(g.key+': virtual row without VA');
            return BigInt(a.dataset.address)>BigInt(b.dataset.address)?-1:BigInt(a.dataset.address)<BigInt(b.dataset.address)?1:0;
          }).forEach(row=>g.node.append(row));
        }
        for(const [key,row]of g.rows) {
          row.style.position='absolute';row.style.left='0';row.style.right='0';
          const i=first.keys.indexOf(key);
          row.style.top=(53+Math.max(0,i)*first.cell)+'px';row.style.height=first.cell+'px';
          if(i<0){row.style.visibility='hidden';row.style.opacity='0';}
        }
        g.node.style.height=(55+first.keys.length*first.cell)+'px';
      }
      const events=[];
      for(const entry of g.entries) {
        const chapter=chapters.find(s=>s.root===entry.sceneRoot), tl=chapter.timeline, at=entry.at||0;
        if(!chapter)fail('member scene missing');
        if(!entry.first) {
          const move=entry.move??1.1;
          const destination={x:entry.left-g.base.left,y:entry.top-g.base.top,scale:1,duration:move,ease:'power2.inOut'};
          if(entry.width)destination.width=entry.width;
          if(entry.via) {
            const via={x:entry.via[0]-g.base.left,y:entry.via[1]-g.base.top,duration:move/2,ease:'power2.inOut'};
            if(entry.width)via.width=entry.width;
            tl.to(g.node,via,at);tl.to(g.node,{...destination,duration:move/2},at+move/2);
          } else tl.to(g.node,destination,at);
        }
        if(g.kind==='array') {
          number(tl,leaf(g.node,'.array-header strong'),entry.header[0],at);
          number(tl,leaf(g.node,'.array-header span'),entry.header[1],at);
          for(const [key,row]of g.rows) {
            const i=entry.keys.indexOf(key), previous=g.entries[g.entries.indexOf(entry)-1];
            if(i<0) {tl.set(row,{autoAlpha:0},at);continue;}
            const data=entry.rows[i];
            value(tl,leaf(row,'.array-name'),data.label,at,true);
            value(tl,leaf(row,'.array-address'),data.address,at,true);
            value(tl,leaf(row,'.array-value'),data.value,at,entry.first);
            const vars={top:53+i*entry.cell,height:entry.cell,duration:entry.first?0:entry.move??1.1,ease:'power2.inOut'};
            tl.to(row,vars,at);
            if(!entry.first&&(!previous.keys.includes(key))&&!data.animated)tl.set(row,{autoAlpha:1},at);
          }
          tl.to(g.node,{height:55+entry.keys.length*entry.cell,duration:entry.first?0:entry.move??1.1},at);
        } else {
          entry.snapshot.forEach(p=>value(tl,p.target,p.value,at,entry.first));
          if(entry.tlb&&!entry.first)tl.set(entry.tlb,{autoAlpha:0},at);
        }
        events.push({scene:chapter.root.id,selector:entry.selector,time:chapter.start+at*stretch,at,keys:entry.keys||null});
      }
      const endChapter=chapters.find(s=>s.root===last.sceneRoot),end=endChapter.start+endChapter.duration;
      if(end<duration-.001)endChapter.timeline.set(g.node,{autoAlpha:0},endChapter.duration/stretch);
      output.push({key:g.key,kind:g.kind,rootId:g.node.id,rows:[...g.rows].map(([key,node])=>({key,id:node.id})),events,end});
    }
    return output;
  }
  return {bind,keepVisible,finish};
}
