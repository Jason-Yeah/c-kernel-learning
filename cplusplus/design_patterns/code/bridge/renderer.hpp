#pragma once

// ══════════════ 实现 (Implementor) ══════════════
// 模式角色：Implementor —— 定义渲染接口，不知道"形状"的存在
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void RenderCircle(float radius, float x, float y) = 0;
    virtual void RenderRectangle(float w, float h) = 0;
};

// ══════════════ 具体实现 A (ConcreteImplementor) ══════════════
// 模式角色：ConcreteImplementor —— 矢量渲染（声明放头文件，实现在 .cpp）
class VectorRenderer : public Renderer {
public:
    void RenderCircle(float radius, float x, float y) override;
    void RenderRectangle(float w, float h) override;
};

// ══════════════ 具体实现 B (ConcreteImplementor) ══════════════
// 模式角色：ConcreteImplementor —— 像素渲染
class RasterRenderer : public Renderer {
public:
    void RenderCircle(float radius, float x, float y) override;
    void RenderRectangle(float w, float h) override;
};
