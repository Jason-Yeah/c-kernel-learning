import { mountExperimentalFilm as mountFilm } from "../shared/experimental-film.js";
import { buildCall } from "./stack-exhaustion.scene-call.js";
import { buildRecursion } from "./stack-exhaustion.scene-recurse.js";
import { buildCapacity } from "./stack-exhaustion.scene-capacity.js";
import { buildOverflow } from "./stack-exhaustion.scene-overflow.js";
import { buildReturn } from "./stack-exhaustion.scene-return.js";
mountFilm(
  {
    id: "stack-exhaustion",
    chapter: 3,
    title: "调用栈为什么会溢出？",
    stretch: 1.5,
    model: "栈帧大小与容量为示意 · 非缓冲区越界写 · 有终止条件也要控制深度",
    steps: [
      2.12, 4.18, 4.98, 5.82, 6.4, 8.9, 9.85, 12.4, 13.6, 15.94, 16.65, 17.37,
      18.05, 19.1,
    ],
    scenes: [
      {
        title: "一次调用",
        detail: "建立栈帧",
        duration: 3,
        view: "CALL VIEW",
        build: buildCall,
      },
      {
        title: "再次调用",
        detail: "等待与累积",
        duration: 4,
        view: "RECURSION / TRACKING",
        build: buildRecursion,
      },
      {
        title: "空间有限",
        detail: "拉远看全局",
        duration: 4,
        view: "STACK / WIDE VIEW",
        build: buildCapacity,
      },
      {
        title: "触及上限",
        detail: "新的调用失败",
        duration: 4,
        view: "STACK LIMIT / CLOSE-UP",
        build: buildOverflow,
      },
      {
        title: "正常返回",
        detail: "另一条路径对照",
        duration: 5,
        view: "ALTERNATIVE / RETURN",
        build: buildReturn,
      },
    ],
  },
  "stack-recursion",
);
