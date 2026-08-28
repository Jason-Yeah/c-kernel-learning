#include "renderer.hpp"
#include <iostream>

// ══════════════ VectorRenderer 实现 ══════════════
void VectorRenderer::RenderCircle(float radius, float x, float y) {
    std::cout << "  [矢量] 画圆: 半径 " << radius
              << " @ (" << x << ", " << y << ")" << std::endl;
}

void VectorRenderer::RenderRectangle(float w, float h) {
    std::cout << "  [矢量] 画矩形: " << w << " × " << h << std::endl;
}

// ══════════════ RasterRenderer 实现 ══════════════
void RasterRenderer::RenderCircle(float radius, float x, float y) {
    std::cout << "  [像素] 画圆: 半径 " << radius
              << " @ (" << x << ", " << y << ")  逐像素填充" << std::endl;
}

void RasterRenderer::RenderRectangle(float w, float h) {
    std::cout << "  [像素] 画矩形: " << w << " × " << h
              << "  逐像素填充" << std::endl;
}
