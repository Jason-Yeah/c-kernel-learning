// Geometry audit of the real rendered stage, including persistent actors.
export async function reviewVisibility(page){return page.evaluate(()=>{
 const host=document.querySelector('#scene-host'),stage=document.querySelector('#film-stage').getBoundingClientRect(),scale=stage.width/1440;
 const visible=el=>{for(let n=el;n&&n!==host;n=n.parentElement){const s=getComputedStyle(n);if(s.display==='none'||s.visibility==='hidden'||Number(s.opacity)<.5)return false;}return !!el.getClientRects().length;};
 const glyphs=[],walker=document.createTreeWalker(host,NodeFilter.SHOW_TEXT);
 while(walker.nextNode()){const n=walker.currentNode;if(!n.textContent.trim()||n.parentElement.closest('svg')||!visible(n.parentElement))continue;const r=document.createRange();r.selectNodeContents(n);for(const box of r.getClientRects())if(box.width>1&&box.height>1)glyphs.push({el:n.parentElement,text:n.textContent.trim(),r:box});}
 const bounds=glyphs.filter(({r})=>(r.left-stage.left)/scale<15||(r.right-stage.left)/scale>1425||(r.top-stage.top)/scale<72||(r.bottom-stage.top)/scale>770).map(g=>g.text);
 const intersect=(a,b)=>Math.min(a.right,b.right)-Math.max(a.left,b.left)>1&&Math.min(a.bottom,b.bottom)-Math.max(a.top,b.top)>2;
 const moving=[...host.querySelectorAll('.moving-packet,.packet')].filter(visible);
 const motion=moving.flatMap(p=>glyphs.filter(g=>intersect(p.getBoundingClientRect(),g.r)).map(g=>[p.id,g.text]));
 const panels=[...host.querySelectorAll('.register-cell,.ibit-cell,.ibit-fold-value,.ibit-ribbon-cell,.ibit-blocked,.code-snippet,.vertical-array,.ec-copy,.ec-process,.ec-node,.ec-irq')].filter(visible);
 const opaque=glyphs.flatMap(g=>panels.filter(p=>!p.contains(g.el)&&intersect(p.getBoundingClientRect(),g.r)).map(p=>[p.id||p.closest('[id]')?.id,g.text]));
 const text=[];for(let i=0;i<glyphs.length;i++)for(let j=i+1;j<glyphs.length;j++){const a=glyphs[i],b=glyphs[j],w=Math.min(a.r.right,b.r.right)-Math.max(a.r.left,b.r.left),h=Math.min(a.r.bottom,b.r.bottom)-Math.max(a.r.top,b.r.top);if(w>3&&h>6&&w*h>50)text.push([a.text,b.text]);}
 const hit=[];for(const g of glyphs)for(const part of[.2,.5,.8]){const target=document.elementFromPoint(g.r.left+g.r.width/2,g.r.top+g.r.height*part);if(target?.closest('.ec-mode')&&!g.el.contains(target)){hit.push([g.text,target.closest('.ec-mode').id]);break;}}
 return{bounds,motion,opaque,text,hit};
});}
