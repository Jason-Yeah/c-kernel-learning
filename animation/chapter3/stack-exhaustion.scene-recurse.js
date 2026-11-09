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
export function buildRecursion() {
  const frames = Array.from({ length: 4 }, (_, i) =>
    frame(`recurse-frame-${i}`, `f(${i + 1})`, {
      value: recursionEnters[i].fields.savedReturn,
      x: 850,
      y: 285 + i * 124,
      index: String(i + 1).padStart(2, "0"),
      animated: i !== 0,
    }),
  ).join("");
  const routes = [0, 1, 2]
    .map(
      (i) =>
        `<g id="recurse-route-${i}" data-anim><path id="recurse-path-${i}" class="flow-path" d="M1178 ${317 + i * 124} C1248 ${317 + i * 124} 1248 ${441 + i * 124} 1178 ${441 + i * 124}"/></g><circle id="recurse-packet-${i}" class="packet" r="6" data-anim/>`,
    )
    .join("");
  const waits = [0, 1, 2]
    .map(
      (i) =>
        `<div id="wait-${i}" class="wait-label" style="left:707px;top:${330 + i * 124}px" data-anim>等待返回</div>`,
    )
    .join("");
  const { root, $, $$ } = scene(
    "scene-recurse",
    "scene-two",
    `
    <div class="camera" id="recurse-camera">${frames}<svg class="world-svg" aria-hidden="true">${routes}</svg>${waits}<div id="recurse-active" class="active-label" style="left:696px;top:696px" data-anim>正在执行 →</div></div>
    <div id="recurse-text" class="positioned" style="left:190px;top:238px" data-anim><div class="eyebrow">RECURSIVE CALL</div><h2 class="scene-heading" style="margin-top:23px">还没返回，<br>又调用了<em>自己</em>。</h2><p class="scene-description">旧的调用等待着，<br>新的栈帧继续建立。</p></div>
    <div id="recurse-caption" class="caption" data-anim>上一层没有结束，下一层已经开始。</div>`,
  );
  const tl = gsap.timeline({ paused: true });
  tl.set($("#recurse-frame-0 .frame-active"), { opacity: 1 }, 0);
  tl.to(
    $("#recurse-camera"),
    { x: -70, y: -135, duration: 0.45, ease: "power2.inOut" },
    0,
  );
  reveal(tl, $("#recurse-text"), 0.15, { x: -28, y: 0, duration: 0.5 });
  reveal(tl, $("#recurse-caption"), 0.3, { duration: 0.35, y: 6 });
  for (let i = 0; i < 3; i++) {
    const at = 0.48 + i * 0.8;
    reveal(tl, $(`#recurse-route-${i}`), at, { y: 0, duration: 0.05 });
    draw(tl, $(`#recurse-path-${i}`), at, 0.48);
    travel(
      tl,
      $(`#recurse-packet-${i}`),
      $(`#recurse-path-${i}`),
      at + 0.02,
      0.5,
    );
    reveal(tl, $(`#recurse-frame-${i + 1}`), at + 0.22, {
      y: -20,
      duration: 0.42,
    });
    tl.to(
      $(`#recurse-frame-${i} .frame-active`),
      { opacity: 0, duration: 0.22 },
      at + 0.35,
    );
    tl.to(
      $(`#recurse-frame-${i}`),
      { opacity: 0.54, duration: 0.25 },
      at + 0.4,
    );
    tl.to(
      $(`#recurse-frame-${i + 1} .frame-active`),
      { opacity: 1, duration: 0.2 },
      at + 0.42,
    );
    reveal(tl, $(`#wait-${i}`), at + 0.5, { x: 8, y: 0, duration: 0.25 });
  }
  reveal(tl, $("#recurse-active"), 2.6, { x: -8, y: 0, duration: 0.3 });
  vanish(tl, $("#recurse-text"), 3.5, { x: -25, y: 0, duration: 0.4 });
  vanish(tl, $("#recurse-caption"), 3.55, { y: 0 });
  vanish(tl, $$(".wait-label, .active-label"), 3.55, { y: 0 });
  vanish(tl, $$(".world-svg"), 3.5, { y: 0, duration: 0.3 });
  vanish(tl, $$(".frame-content, .frame-top-rule"), 3.5, {
    y: 0,
    duration: 0.3,
  });
  for (let i = 0; i < 4; i++) {
    tl.to(
      $(`#recurse-frame-${i}`),
      {
        height: 58,
        y: -58 * i,
        opacity: 1,
        duration: 0.5,
        ease: "power2.inOut",
      },
      3.5,
    );
  }
  // Match the next shot's first four compact frames, then let that camera pull back.
  tl.to(
    $("#recurse-camera"),
    { x: -517.5, y: -203.75, scale: 1.15, duration: 0.5, ease: "power2.inOut" },
    3.5,
  );
  return { root, timeline: finish(tl, 4) };
}
