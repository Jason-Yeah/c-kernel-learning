import {
  gsap,
  scene,
  frame,
  reveal,
  vanish,
  draw,
  travel,
  finish,
} from "../shared/scene-kit.js";

import { recursionEnters } from "../models/experiments.js";
export function buildCall() {
  const { root, $ } = scene(
    "scene-call",
    "scene-one",
    `
    <div class="camera">
      <div class="function-chip" id="call-function"><div class="function-code">f<span class="parens">(1)</span></div><div class="function-caption">FUNCTION CALL</div><i class="function-cursor" data-anim></i></div>
      <svg class="world-svg" aria-hidden="true"><g id="call-route" data-anim><path id="call-path" class="flow-path" d="M494 354 C620 354 697 341 830 341"/></g><circle id="call-packet" class="packet" r="7" data-anim/></svg>
      ${frame("call-frame", "f(1)", { x: 850, y: 285, value: recursionEnters[0].fields.savedReturn })}
      <div id="call-frame-label" class="positioned local-note" style="left:850px;top:444px" data-anim>一个调用，一份<strong>栈帧</strong>。</div>
      <div id="call-return-label" class="positioned tiny-note" style="left:850px;top:486px" data-anim>保存返回位置与本次调用的状态</div>
    </div>
    <div class="caption" id="call-caption" data-anim>每次函数调用，都会占用一份栈空间。</div>`,
  );
  const tl = gsap.timeline({ paused: true });
  tl.fromTo(
    $("#call-function"),
    { x: 0, scale: 0.97 },
    { scale: 1, duration: 0.4, ease: "power2.out", immediateRender: false },
    0,
  );
  tl.to(
    $("#call-function"),
    { x: -310, duration: 0.7, ease: "power3.inOut" },
    0.35,
  );
  reveal(tl, $(".function-cursor"), 0.5, { duration: 0.2, y: 0 });
  reveal(tl, $("#call-route"), 0.78, { y: 0, duration: 0.05 });
  draw(tl, $("#call-path"), 0.78, 0.65);
  travel(tl, $("#call-packet"), $("#call-path"), 0.88, 0.65);
  reveal(tl, $("#call-frame"), 1.18, {
    x: 0,
    y: 0,
    scale: 0.92,
    duration: 0.5,
  });
  tl.to($("#call-frame .frame-active"), { opacity: 1, duration: 0.3 }, 1.45);
  reveal(tl, $("#call-frame-label"), 1.6, { y: 10, duration: 0.3 });
  reveal(tl, $("#call-return-label"), 1.78, { y: 8, duration: 0.3 });
  reveal(tl, $("#call-caption"), 1.05, { y: 8, duration: 0.3 });
  vanish(tl, $("#call-function"), 2.55, { x: -340, y: 0 });
  vanish(tl, $("#call-route"), 2.6, { y: 0 });
  vanish(tl, $("#call-frame-label"), 2.6);
  vanish(tl, $("#call-return-label"), 2.6);
  vanish(tl, $("#call-caption"), 2.65, { y: 0 });
  return { root, timeline: finish(tl, 3) };
}
