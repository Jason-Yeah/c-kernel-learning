#include "shape.hpp"

// ══════════════ Abstraction 实现 ══════════════
Shape::Shape(Renderer& r) : renderer_(r) {}

// ══════════════ Circle 实现 ══════════════
Circle::Circle(Renderer& r, float radius, float x, float y)
    : Shape(r), radius_(radius), x_(x), y_(y) {}

void Circle::Draw() const {
    // ★ 形状不自己画，委托给桥另一端的 Renderer
    renderer_.RenderCircle(radius_, x_, y_);
}

void Circle::Scale(float factor) {
    radius_ *= factor;
}

// ══════════════ Rectangle 实现 ══════════════
Rectangle::Rectangle(Renderer& r, float w, float h)
    : Shape(r), width_(w), height_(h) {}

void Rectangle::Draw() const {
    renderer_.RenderRectangle(width_, height_);
}

void Rectangle::Scale(float factor) {
    width_ *= factor;
    height_ *= factor;
}
