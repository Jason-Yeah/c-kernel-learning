# 旧版清理记录

## 清理前提

此前七部新版的模型、47 镜头、播放控制、完整原速播放和静态子路径验证全部通过后，才执行旧版清理。现维护唯一的 HTML + 多组件 SVG + GSAP 架构。本页记录那次清理基线；本轮补充 C／GDB 后已为 53 镜头，最新状态见 [spec.md](spec.md)。

## 已删除的 20 个被替代源文件

- 六个旧 SVG：chapter3 的 stack-frame、stack-overflow；chapter6 的 cache；chapter8 的 process-lifecycle、fork-memory-layout；chapter9 的 address-translation。
- 上述六个主题的 `.build.py` 生成器，以及 chapter3 的 `stack_scene.py`。
- 旧共享 SMIL 播放器 `svg_player.py`、`svg-player.js` 与 `svg-player.test.cjs`。
- 旧 fork-memory-layout SVG 测试。
- 栈耗尽样片的重复 CSS、重复动画工具与旧专用测试，统一由 shared 和 tests 中的公共实现替代。

旧版源文件已在清理前做临时安全快照：`/tmp/csapp-animation-legacy-20261105-111449.tgz`。它不在项目、发布目录或网站中；/tmp 不是长期备份，不承诺跨系统清理保留。

## 保留与更新

- 七个当前主题及栈耗尽的五个分镜模块保留，统一使用 `shared/player.js`、`shared/scene-kit.js` 和共享样式。
- `chapter8/fork-memory-demo.c` 保留，只有注释中的动画链接更新；实际程序逻辑不变并重新编译运行验证。
- AGENTS、规划、README、样片说明和 C 示例说明已改为新入口与新工作流；章节与主题链接只指向当前 HTML。
- 旧版样片截图和重复录屏临时目录已清理，当前七部录屏及新接触表保留在忽略版本管理的 film-artifacts 中。
- 不修改本项目之外的学习代码、笔记、VS Code 配置或其他工程。
