import { gsap } from "../node_modules/gsap/index.js";
import { MotionPathPlugin } from "../node_modules/gsap/MotionPathPlugin.js";
gsap.registerPlugin(MotionPathPlugin, {
  name: "exactText",
  init(target, value) {
    this.target = target;
    this.before = target.textContent;
    this.after = String(value);
  },
  render(ratio, data) {
    data.target.textContent = ratio ? data.after : data.before;
  },
});
export { gsap };
export const COLORS = {
  cyan: "#69e2f4",
  blue: "#64a4ff",
  red: "#ff667b",
  ink: "#ffffff",
  muted: "#ffffff",
};
export const hex = (value, width = 2) =>
  value.toString(16).toUpperCase().padStart(width, "0");
export const escape = (value) =>
  String(value).replace(
    /[&<>"']/g,
    (c) =>
      ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[
        c
      ],
  );

export function frame(
  id,
  label,
  {
    x,
    y,
    width = 320,
    height = 112,
    index = "01",
    compact = false,
    extra = "",
    animated = true,
    content = "返回位置",
    value = "caller + …",
  } = {},
) {
  const body = compact
    ? `<div class="mini-label">${label}</div><div class="mini-index">#${index}</div>`
    : `<div class="frame-top"><span>${label}</span><span class="frame-index">#${index}</span></div><div class="frame-content"><span>${content}</span><code>${value}</code></div>`;
  const shape = `M0 0H${width - 12}L${width} 12V${height}H0Z`;
  return `<div id="${id}" class="stack-frame ${compact ? "mini-frame" : ""} ${extra}" ${animated ? "data-anim" : ""} style="left:${x}px;top:${y}px;width:${width}px;height:${height}px"><svg class="frame-shape" viewBox="0 0 ${width} ${height}" preserveAspectRatio="none" aria-hidden="true"><path class="frame-body" d="${shape}"/>${compact ? "" : `<path class="frame-top-rule" d="M0 40H${width}"/>`}<path class="frame-active" d="${shape}"/></svg>${body}<span class="frame-port"></span></div>`;
}
let actorContext = null;
export function setActorContext(context) { actorContext = context; }
export function scene(id, className, markup) {
  const root = document.createElement("section");
  root.id = id;
  root.className = `scene ${className}`;
  root.hidden = true;
  root.setAttribute("aria-hidden", "true");
  root.innerHTML = markup;
  document.querySelector("#scene-host").append(root);
  const binding = actorContext?.bind(id, root);
  return {
    root,
    $: (selector) => binding ? binding.query(selector) : root.querySelector(selector),
    $$: (selector) => binding ? binding.query(selector, true) : [...root.querySelectorAll(selector)],
  };
}
// A shared actor layer, not another local scene: keep identities across cuts.
// Authors must explicitly animate initial visibility and retirement on its timeline.
export function continuity(id, markup, duration) {
  const root = document.createElement("section");
  root.id = id;
  root.className = "film-continuity";
  root.dataset.filmContinuity = "";
  root.setAttribute("aria-label", "跨镜头保留的教学对象");
  root.innerHTML = markup;
  document.querySelector("#scene-host").append(root);
  return { root, duration, timeline: gsap.timeline({ paused: true }),
    $: selector => root.querySelector(selector),
    $$: selector => [...root.querySelectorAll(selector)] };
}
export function reveal(
  tl,
  target,
  at,
  { duration = 0.65, x = 0, y = 16, scale = 1, ease = "power2.out" } = {},
) {
  if (actorContext?.keepVisible(target)) return;
  tl.fromTo(
    target,
    { autoAlpha: 0, x, y, scale },
    {
      autoAlpha: 1,
      x: 0,
      y: 0,
      scale: 1,
      duration,
      ease,
      immediateRender: false,
    },
    at,
  );
}
export function vanish(
  tl,
  target,
  at,
  { duration = 0.45, x = 0, y = -12 } = {},
) {
  tl.to(target, { autoAlpha: 0, x, y, duration, ease: "power2.inOut" }, at);
}
export function draw(tl, path, at, duration = 0.7) {
  const length = path.getTotalLength();
  tl.fromTo(
    path,
    { strokeDasharray: length, strokeDashoffset: length },
    {
      strokeDashoffset: 0,
      duration,
      ease: "power2.inOut",
      immediateRender: false,
    },
    at,
  );
}
export function travel(tl, token, path, at, duration = 0.9) {
  const p = path.getPointAtLength(0);
  tl.set(token, { autoAlpha: 1 }, at);
  // Raw local coordinates work even while a future scene is display:none.
  tl.fromTo(
    token,
    { x: p.x, y: p.y },
    {
      motionPath: { path: path.getAttribute("d") },
      duration,
      ease: "power1.inOut",
      immediateRender: false,
    },
    at,
  );
  tl.set(token, { autoAlpha: 0 }, at + duration);
}
export function finish(tl, duration) {
  tl.to({}, { duration: 0.001 }, duration - 0.001);
  return tl;
}
export function shot(id, markup, duration, { stops = [], checks = [] } = {}) {
  const s = scene(id, "directed-scene", markup);
  return {
    ...s,
    timeline: gsap.timeline({ paused: true }),
    duration,
    stops,
    checks,
  };
}
export function closeShot(s, { exit = true } = {}) {
  if (exit) {
    // Only the camera layer exits: scene-bound captions and annotations are gated too.
    const camera = s.$(".camera");
    if (camera)
      s.timeline.to(
        camera,
        { opacity: 0.12, y: -10, duration: 0.6, ease: "power2.inOut" },
        s.duration - 0.6,
      );
  }
  finish(s.timeline, s.duration);
  return s;
}
export function pos(
  id,
  x,
  y,
  content,
  cls = "",
  { visible = false, width } = {},
) {
  return `<div id="${id}" class="positioned ${cls}" ${visible ? "" : "data-anim"} style="left:${x}px;top:${y}px;${width ? `width:${width}px;` : ""}">${content}</div>`;
}
export function heading(id, x, y, eyebrow, title, description = "") {
  return pos(
    id,
    x,
    y,
    `<div class="eyebrow">${eyebrow}</div><h2 class="scene-heading" style="margin-top:20px">${title}</h2>${description ? `<p class="scene-description">${description}</p>` : ""}`,
    "shot-heading",
  );
}
export function note(
  id,
  x,
  y,
  title,
  detail = "",
  { red = false, width = 380 } = {},
) {
  return pos(
    id,
    x,
    y,
    `<strong>${title}</strong>${detail ? `<span>${detail}</span>` : ""}`,
    `anchored-note ${red ? "red-note" : ""}`,
    { width },
  );
}
export function code(
  id,
  x,
  y,
  lines,
  { title = "C / TEACHING EXAMPLE", width = 480, lineNumbers = null } = {},
) {
  return pos(
    id,
    x,
    y,
    `<div class="code-heading">${title}</div><pre>${lines.map((line, i) => `<span class="code-line" data-line="${i}" data-source-line="${lineNumbers?.[i] ?? ""}"><i>${lineNumbers?.[i] ?? i + 1}</i><code>${escape(line)}</code></span>`).join("")}</pre>`,
    "code-snippet",
    { width },
  );
}
export function activeCode(s, line, at) {
  const lines = s.$$(".code-line");
  lines.forEach((el) =>
    s.timeline.to(
      el,
      {
        color: Number(el.dataset.line) === line ? COLORS.cyan : COLORS.ink,
        backgroundColor:
          Number(el.dataset.line) === line ? "#69e2f40b" : "#69e2f400",
        duration: 0.25,
      },
      at,
    ),
  );
}
export function register(
  id,
  x,
  y,
  name,
  value,
  { width = 240, visible = false } = {},
) {
  const parts = name.match(/^(%[a-z][a-z0-9]*)(?:\s*[·/]\s*(.*))?$/i);
  const label = parts
    ? `<strong class="reg-name">${escape(parts[1])}</strong>${parts[2] ? ` <span class="reg-detail">· ${escape(parts[2])}</span>` : ""}`
    : escape(name);
  return pos(
    id,
    x,
    y,
    `<span class="reg-label">${label}</span><b class="reg-value">${value}</b>`,
    "register-cell",
    { width, visible },
  );
}
// Discrete text plugin preserves hex, leading zeroes and reverse-seek state.
// GSAP's numeric textContent interpolation would turn 0x7FF8 into 32760.
export function number(tl, el, value, at) {
  tl.set(el, { exactText: String(value) }, at);
}
export function chip(
  id,
  x,
  y,
  { label = "CPU", subtitle = "同一颗处理器", width = 260 } = {},
) {
  const pins = Array.from({ length: 8 }, (_, i) => {
    const p = 32 + i * 22;
    return `<path d="M${p} 2v16M${p} 202v16M2 ${p}h16M202 ${p}h16"/>`;
  }).join("");
  return pos(
    id,
    x,
    y,
    `<svg class="chip-svg" viewBox="0 0 220 220" aria-hidden="true"><g class="chip-pins">${pins}</g><rect x="18" y="18" width="184" height="184" rx="14"/><rect class="chip-die" x="39" y="39" width="142" height="142" rx="8"/></svg><div class="chip-label">${label}</div><div class="chip-subtitle">${subtitle}</div>`,
    "cpu-chip",
    { width },
  );
}
export function page(
  id,
  x,
  y,
  label,
  value,
  { width = 220, visible = false, extra = "" } = {},
) {
  return pos(
    id,
    x,
    y,
    `<svg viewBox="0 0 220 178" preserveAspectRatio="none" class="page-shape" aria-hidden="true"><path d="M1 1H191L219 29V177H1Z M191 1V29H219"/></svg><div class="page-label">${label}</div><div class="page-value">${value}</div>`,
    `memory-page ${extra}`,
    { width, visible },
  );
}
export function windowBox(
  id,
  x,
  y,
  title,
  content,
  { width = 360, visible = false, extra = "" } = {},
) {
  return pos(
    id,
    x,
    y,
    `<div class="window-title"><i></i><i></i><i></i><span>${title}</span></div><div class="window-body">${content}</div>`,
    `program-window ${extra}`,
    { width, visible },
  );
}
export function ribbon(
  id,
  x,
  y,
  cells,
  { cellWidth = 43, labels = true, prefix = id } = {},
) {
  return pos(
    id,
    x,
    y,
    cells
      .map(
        (v, i) =>
          `<div id="${prefix}-${i}" class="byte-cell" style="width:${cellWidth}px"><b>${v === null ? "··" : escape(v)}</b>${labels ? `<small>${i}</small>` : ""}</div>`,
      )
      .join(""),
    "byte-ribbon",
  );
}
export function wires(id, routes, { packetRadius = null } = {}) {
  return `<svg class="world-svg" id="${id}" aria-hidden="true">${routes.map(([key, path, color = "cyan", label = ""]) => `<g id="${key}-route" data-anim><path id="${key}-path" class="flow-path" style="stroke:${COLORS[color]}" d="${path}"/></g><g id="${key}-packet" class="moving-packet" data-anim><circle r="${packetRadius ?? (label ? 22 : 7)}" fill="${COLORS[color]}"/>${label ? `<text text-anchor="middle" y="6">${escape(label)}</text>` : ""}</g>`).join("")}</svg>`;
}
export function flow(s, key, at, duration = 1, { hide = true } = {}) {
  reveal(s.timeline, s.$(`#${key}-route`), at, { duration: 0.05, y: 0 });
  draw(s.timeline, s.$(`#${key}-path`), at, duration);
  travel(
    s.timeline,
    s.$(`#${key}-packet`),
    s.$(`#${key}-path`),
    at + 0.08,
    duration,
  );
  if (hide)
    vanish(s.timeline, s.$(`#${key}-route`), at + duration + 0.8, {
      duration: 0.4,
      y: 0,
    });
}
export function state(s, phase, at) {
  s.timeline.set(s.root, { attr: { "data-phase": phase } }, at);
}
export function camera(
  s,
  at,
  { x = 0, y = 0, scale = 1, duration = 1.2 } = {},
) {
  s.timeline.to(
    s.$(".camera"),
    { x, y, scale, duration, ease: "power2.inOut", transformOrigin: "0 0" },
    at,
  );
}
export function caption(id, text) {
  return `<div id="${id}" class="caption" data-anim>${text}</div>`;
}
