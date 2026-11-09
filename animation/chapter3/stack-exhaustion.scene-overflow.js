import {
  gsap,
  scene,
  frame,
  reveal,
  vanish,
  travel,
  finish,
} from "../shared/scene-kit.js";

import {
  recursionDepth,
  recursionEnters,
  recursionFault,
} from "../models/experiments.js";
export function buildOverflow() {
  const { root, $ } = scene(
    "scene-overflow",
    "scene-four",
    `
    <div class="camera" id="overflow-camera">
      <div id="overflow-kicker" class="positioned eyebrow" style="left:280px;top:125px" data-anim>LAST AVAILABLE SPACE</div>
      ${frame("overflow-valid", `f(${recursionDepth})`, { x: 280, y: 175, width: 560, height: 124, index: String(recursionDepth), animated: false, value: recursionEnters.at(-1).fields.savedReturn })}
      <div id="overflow-last-label" class="positioned tiny-note" style="left:280px;top:309px" data-anim>最后一个有效栈帧</div>
      <div id="overflow-hatch" class="danger-hatch" style="left:250px;top:357px;width:620px;height:218px" data-anim></div>
      ${frame("overflow-ghost", `f(${recursionDepth + 1}) · 未完成`, { x: 280, y: 340, width: 560, height: 132, index: String(recursionDepth + 1), extra: "ghost-frame", content: "本次序言访问栈触发信号", value: "pending" })}
      <svg class="world-svg" aria-hidden="true"><path id="overflow-limit-neutral" class="limit-line" d="M250 357H870"/><path id="overflow-limit-danger" class="danger-line" d="M250 357H870" data-anim/><path id="overflow-request-path" d="M860 205 C910 205 920 330 873 351" fill="none" stroke="#3d566c" stroke-width="1.5" data-anim/><circle id="overflow-request" class="packet" r="7" data-anim/></svg>
      <div id="overflow-attempt-note" class="positioned tiny-note" style="left:280px;top:493px" data-anim>虚线为分配尝试，不代表已经占用</div>
      <div id="overflow-limit-label" class="positioned limit-label" style="left:280px;top:599px" data-anim>STACK LIMIT / 无法容纳新的栈帧</div>
      <svg id="overflow-cross" class="failure-cross" style="left:858px;top:338px" viewBox="0 0 34 34" aria-hidden="true" data-anim><path d="m10 10 14 14m0-14L10 24"/></svg>
    </div>
    <div id="overflow-error" class="positioned" style="left:957px;top:264px" data-anim><div class="eyebrow" style="color:#ff667b">STACK ACCESS FAULT</div><div class="error-title" style="margin-top:19px">STACK<br>OVERFLOW</div><div class="error-subtitle">GDB：SIGSEGV</div><div class="error-code">si_addr = ${recursionFault.signal.address}</div></div>
    <div id="overflow-caption" class="caption" data-anim>实测停在新一层的栈访问，栈帧没有完整建立。</div>
    <div id="overflow-rewind" class="positioned rewind-badge" style="left:563px;top:617px" data-anim><svg viewBox="0 0 24 24"><path d="m12 6-8 6 8 6V6zm8 0-8 6 8 6V6z"/></svg>回看另一条正常路径</div>`,
  );
  const tl = gsap.timeline({ paused: true });
  tl.set($("#overflow-valid .frame-active"), { opacity: 1 }, 0);
  tl.fromTo(
    $("#overflow-valid"),
    { height: 101.5 },
    { height: 124, duration: 0.32, ease: "power2.out", immediateRender: false },
    0,
  );
  tl.fromTo(
    $("#overflow-limit-neutral"),
    { y: -49 },
    { y: 0, duration: 0.32, ease: "power2.out", immediateRender: false },
    0,
  );
  reveal(tl, $("#overflow-kicker"), 0.08, { y: -5, duration: 0.3 });
  reveal(tl, $("#overflow-last-label"), 0.15, { y: 0, duration: 0.3 });
  reveal(tl, $("#overflow-request-path"), 0.4, { y: 0, duration: 0.1 });
  travel(tl, $("#overflow-request"), $("#overflow-request-path"), 0.45, 0.7);
  reveal(tl, $("#overflow-ghost"), 0.48, {
    x: 0,
    y: -32,
    scale: 0.98,
    duration: 0.65,
    ease: "power2.out",
  });
  tl.fromTo(
    $("#overflow-ghost .frame-body"),
    { stroke: "#8494a6", fill: "#151c2500" },
    {
      stroke: "#ff667b",
      fill: "#ff667b05",
      duration: 0.25,
      immediateRender: false,
    },
    1.05,
  );
  tl.fromTo(
    $("#overflow-ghost .frame-top"),
    { color: "#ffffff" },
    { color: "#ffa2af", duration: 0.25, immediateRender: false },
    1.05,
  );
  tl.fromTo(
    $("#overflow-ghost .frame-content"),
    { opacity: 0.4 },
    { opacity: 1, duration: 0.25, immediateRender: false },
    1.05,
  );
  tl.set(
    $("#overflow-ghost .frame-content code"),
    { textContent: "SIGSEGV" },
    1.15,
  );
  reveal(tl, $("#overflow-limit-danger"), 1.1, { y: 0, duration: 0.15 });
  reveal(tl, $("#overflow-hatch"), 1.15, { y: 0, duration: 0.3 });
  reveal(tl, $("#overflow-cross"), 1.15, { y: 0, scale: 0.6, duration: 0.25 });
  reveal(tl, $("#overflow-limit-label"), 1.25, { y: 8, duration: 0.35 });
  reveal(tl, $("#overflow-attempt-note"), 1.35, { y: 5, duration: 0.3 });
  reveal(tl, $("#overflow-error"), 1.6, { x: 15, y: 0, duration: 0.4 });
  reveal(tl, $("#overflow-caption"), 1.6, { y: 8, duration: 0.35 });
  // One restrained pulse, not a flashing screen or an overwrite of older frames.
  tl.to(
    $("#overflow-limit-danger"),
    {
      strokeWidth: 5,
      duration: 0.13,
      repeat: 1,
      yoyo: true,
      ease: "sine.inOut",
    },
    1.22,
  );
  vanish(tl, $("#overflow-error"), 3.5, { y: 0, x: 20, duration: 0.35 });
  vanish(tl, $("#overflow-caption"), 3.5, { y: 0 });
  vanish(tl, $("#overflow-ghost"), 3.5, { y: -45, duration: 0.4 });
  tl.to(
    $("#overflow-camera"),
    { opacity: 0.12, scale: 0.96, x: 20, y: 12, duration: 0.4 },
    3.55,
  );
  reveal(tl, $("#overflow-rewind"), 3.55, { y: 6, duration: 0.3 });
  return { root, timeline: finish(tl, 4) };
}
