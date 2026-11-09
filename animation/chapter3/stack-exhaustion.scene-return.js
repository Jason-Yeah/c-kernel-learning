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

import { experiments } from "../models/experiments.js";
export function buildReturn() {
  const bounded = experiments["stack-recursion"].runs.bounded.samples.filter(
    (s) => s.phase === 1,
  );
  const frames = bounded
    .map((sample, i) =>
      frame(`return-frame-${i}`, `f(${sample.fields.depth})`, {
        x: 560,
        y: 148 + i * 108,
        height: 90,
        value: sample.fields.savedReturn,
        index: String(i + 1).padStart(2, "0"),
      }),
    )
    .join("");
  const { root, $ } = scene(
    "scene-return",
    "scene-five",
    `
    <div id="return-demonstration">
      <div id="return-path-label" class="positioned rewind-badge" style="left:157px;top:162px" data-anim><svg viewBox="0 0 24 24"><path d="M4 12a8 8 0 1 1 3 6M4 5v7h7"/></svg>正常路径对照<div class="tiny-note" style="position:absolute;top:42px;left:36px;width:235px">加入终止条件，重新执行</div></div>
      <div id="return-camera" class="camera">${frames}
        <div id="return-base-case" class="positioned base-case" style="left:954px;top:480px" data-anim>实测终止条件<br><code>depth == 4</code></div>
        <div id="return-up-label" class="positioned local-note" style="left:954px;top:224px" data-anim><strong>返回</strong>上一层，<br>释放当前栈帧。</div>
        <svg class="world-svg" aria-hidden="true">
          ${Array.from({ length: 4 }, (_, i) => {
            const from = 193 + (3 - i) * 108,
              to = from - 108;
            return `<g id="return-route-${i}" data-anim><path id="return-path-${i}" class="flow-path" d="M548 ${from}C451 ${from} 451 ${to} 548 ${to}"/></g><circle id="return-packet-${i}" class="packet" r="7" data-anim/>`;
          }).join("")}
        </svg>
      </div>
      <div id="return-caption" class="caption" data-anim>不再继续调用，才能开始逐层返回。</div>
    </div>
    <div id="return-summary" class="summary" data-anim>
      <svg class="summary-icon" viewBox="0 0 64 64" aria-hidden="true"><path d="M17 18h30M17 32h30M17 46h30M8 13v38m48-38v38M4 17l4-4 4 4m40 30 4 4 4-4"/></svg>
      <h2>调用<em>入栈</em>，返回<em>出栈</em>。</h2><p>bounded 独立运行：4 层全部返回，实测 sum = 10。</p><div class="summary-rule"></div>
    </div>`,
  );
  const tl = gsap.timeline({ paused: true });
  reveal(tl, $("#return-path-label"), 0, { x: -12, y: 0, duration: 0.4 });
  for (let i = 0; i < 4; i++)
    reveal(tl, $(`#return-frame-${i}`), 0.1 + i * 0.14, {
      y: -20,
      duration: 0.3,
    });
  tl.to(
    $("#return-frame-3 .frame-active"),
    { opacity: 1, duration: 0.2 },
    0.55,
  );
  reveal(tl, $("#return-base-case"), 0.6, { x: 14, y: 0, duration: 0.3 });
  reveal(tl, $("#return-caption"), 0.6, { y: 7, duration: 0.3 });
  reveal(tl, $("#return-up-label"), 1, { x: 12, y: 0, duration: 0.3 });
  for (let i = 0; i < 4; i++) {
    const at = 0.85 + i * 0.5;
    const frameIndex = 3 - i;
    reveal(tl, $(`#return-route-${i}`), at, { y: 0, duration: 0.05 });
    draw(tl, $(`#return-path-${i}`), at, 0.4);
    travel(
      tl,
      $(`#return-packet-${i}`),
      $(`#return-path-${i}`),
      at + 0.02,
      0.47,
    );
    tl.to(
      $(`#return-frame-${frameIndex}`),
      {
        scaleY: 0.35,
        autoAlpha: 0,
        y: -16,
        duration: 0.32,
        ease: "power2.in",
        transformOrigin: "50% 0%",
      },
      at + 0.15,
    );
    if (frameIndex > 0)
      tl.to(
        $(`#return-frame-${frameIndex - 1} .frame-active`),
        { opacity: 1, duration: 0.15 },
        at + 0.42,
      );
    vanish(tl, $(`#return-route-${i}`), at + 0.55, { y: 0, duration: 0.15 });
  }
  vanish(tl, $("#return-base-case"), 1.4, { x: 12, y: 0 });
  tl.to(
    $("#return-camera"),
    { y: -18, duration: 2.3, ease: "sine.inOut" },
    0.8,
  );
  vanish(tl, $("#return-demonstration"), 3.08, { duration: 0.3, y: -12 });
  reveal(tl, $("#return-summary"), 3.4, {
    y: 10,
    scale: 0.985,
    duration: 0.45,
  });
  tl.to(
    $("#return-summary"),
    { opacity: 0.88, duration: 0.4, ease: "sine.out" },
    4.6,
  );
  return { root, timeline: finish(tl, 5) };
}
