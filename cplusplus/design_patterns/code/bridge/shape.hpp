#pragma once

#include "renderer.hpp"

// ══════════════ 抽象 (Abstraction) ══════════════
// 模式角色：Abstraction —— 定义形状接口，持有 Implementor 引用（桥！）
class Shape {
protected:
    Renderer& renderer_;     // ← 桥：抽象持有实现的引用

public:
    explicit Shape(Renderer& r);
    virtual ~Shape() = default;
    virtual void Draw() const = 0;
    virtual void Scale(float factor) = 0;
};

// ══════════════ 扩展抽象 A (RefinedAbstraction) ══════════════
// 模式角色：RefinedAbstraction —— 圆形
class Circle : public Shape {
    float radius_, x_, y_;

public:
    Circle(Renderer& r, float radius, float x, float y);
    void Draw() const override;
    void Scale(float factor) override;
};

// ══════════════ 扩展抽象 B (RefinedAbstraction) ══════════════
// 模式角色：RefinedAbstraction —— 矩形
class Rectangle : public Shape {
    float width_, height_;

public:
    Rectangle(Renderer& r, float w, float h);
    void Draw() const override;
    void Scale(float factor) override;
};
