import {
  gsap,
  scene,
  frame,
  reveal,
  vanish,
  finish,
  number,
} from "../shared/scene-kit.js";

import { recursionDepth, recursionStride } from "../models/experiments.js";
export function buildCapacity() {
  const depths = [
    1,
    2,
    3,
    4,
    Math.round(recursionDepth / 2),
    Math.round((recursionDepth * 2) / 3),
    Math.round((recursionDepth * 5) / 6),
    recursionDepth,
  ];
  const frames = Array.from({ length: 8 }, (_, i) =>
    frame(`capacity-frame-${i}`, `f(${depths[i]})`, {
      x: 460,
      y: 124 + i * 66,
      height: 58,
      index: String(depths[i]).padStart(2, "0"),
      compact: true,
      animated: i > 3,
    }),
  ).join("");
  const { root, $, $$ } = scene(
    "scene-capacity",
    "scene-three",
    `
    <div class="camera" id="capacity-camera">
      <div class="empty-space" id="capacity-space" style="left:460px;top:388px;width:320px;height:274px" data-anim></div>
      ${frames}
      <svg class="world-svg" aria-hidden="true"><path id="capacity-spine" class="stack-spine" d="M428 125h-16v519h16" data-anim/><g id="capacity-limit" data-anim><path class="limit-line" d="M414 662H829"/><path class="bracket" d="M840 648v14h-14"/></g><path id="capacity-leader" class="annotation-line" d="M780 645H875L904 616" data-anim/></svg>
      <div id="capacity-limit-label" class="positioned limit-label" style="left:460px;top:682px" data-anim>STACK LIMIT / 可用空间上限</div>
      <div id="capacity-remaining" class="positioned local-note" style="left:915px;top:580px" data-anim>空间，即将<em style="font-style:normal;color:#ffffff">用尽</em>。<div class="tiny-note" style="margin-top:9px">实测每层约 ${recursionStride}B；只选画 8 帧</div></div>
    </div>
    <div id="capacity-text" class="positioned" style="left:915px;top:202px" data-anim><div class="eyebrow">CALL DEPTH</div><div style="margin-top:18px"><span id="depth-count" class="depth-count">4</span><span class="depth-unit">实测层</span></div><div class="count-rule"></div><p class="scene-description" style="margin-top:30px">调用仍在增加，<br>中间帧折叠，并非只有 8 层。</p></div>
    <div id="capacity-caption" class="caption" data-anim>镜头拉远：栈的可用空间，是有限的。</div>`,
  );
  const tl = gsap.timeline({ paused: true });
  // A moving composition, not an unchanged diagram: settle the pulled-back column.
  tl.fromTo(
    $("#capacity-camera"),
    { x: -69, y: -18.6, scale: 1.15 },
    {
      x: 0,
      y: 0,
      scale: 1,
      duration: 0.5,
      ease: "power2.out",
      immediateRender: false,
    },
    0,
  );
  tl.set($("#capacity-frame-3 .frame-active"), { opacity: 1 }, 0);
  reveal(tl, $("#capacity-text"), 0.2, { x: 30, y: 0, duration: 0.5 });
  reveal(tl, $("#capacity-spine"), 0.25, { y: 0, duration: 0.4 });
  reveal(tl, $("#capacity-space"), 0.35, { y: 0, duration: 0.2 });
  tl.to(
    $("#capacity-space"),
    { top: 644, height: 18, duration: 1.45, ease: "power1.in" },
    0.45,
  );
  for (let i = 4; i < 8; i++) {
    const at = [0.55, 0.96, 1.29, 1.56][i - 4];
    reveal(tl, $(`#capacity-frame-${i}`), at, {
      y: -19,
      scale: 0.985,
      duration: 0.29,
    });
    tl.to(
      $(`#capacity-frame-${i} .frame-active`),
      { opacity: 1, duration: 0.12 },
      at + 0.12,
    );
    tl.to(
      $(`#capacity-frame-${i - 1} .frame-active`),
      { opacity: 0, duration: 0.12 },
      at + 0.12,
    );
    number(tl, $("#depth-count"), depths[i], at + 0.29);
  }
  reveal(tl, $("#capacity-limit"), 2, { y: 12, duration: 0.32 });
  reveal(tl, $("#capacity-limit-label"), 2.1, { y: 7, duration: 0.3 });
  reveal(tl, $("#capacity-leader"), 2.15, { y: 0, duration: 0.3 });
  reveal(tl, $("#capacity-remaining"), 2.22, { y: 8, duration: 0.3 });
  reveal(tl, $("#capacity-caption"), 0.5, { y: 8, duration: 0.3 });
  vanish(tl, $("#capacity-text"), 3.5, { x: 30, y: 0 });
  vanish(tl, $("#capacity-caption"), 3.55, { y: 0 });
  vanish(tl, $("#capacity-remaining"), 3.55, { x: 20, y: 0 });
  vanish(tl, $("#capacity-limit-label"), 3.55, { y: 0 });
  // Push into the last valid frame and the limit; the next shot keeps this focus.
  tl.to(
    $("#capacity-camera"),
    { scale: 1.75, x: -525, y: -850.5, duration: 0.5, ease: "power2.inOut" },
    3.5,
  );
  tl.to($$(".mini-frame").slice(0, 6), { opacity: 0.25, duration: 0.3 }, 3.6);
  return { root, timeline: finish(tl, 4) };
}
