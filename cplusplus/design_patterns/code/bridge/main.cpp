#include "renderer.hpp"
#include "shape.hpp"
#include <iostream>

// ══════════════ 客户端 (Client) ══════════════
// 模式角色：Client —— 自由组合形状和渲染方式
// 注意：这里直接 include renderer.hpp 拿到具体渲染器类，
// 实际工程通常由工厂/依赖注入创建，main 只负责组装
int main()
{
    // 创建两种渲染器（实现维度）
    VectorRenderer vector;
    RasterRenderer raster;

    // ★ 自由组合：同一个圆形，用不同的渲染器
    Circle vectorCircle(vector, 5, 0, 0); // 矢量圆
    Circle rasterCircle(raster, 5, 0, 0); // 像素圆

    std::cout << "=== 圆形 + 矢量渲染 ===" << std::endl;
    vectorCircle.Draw();
    std::cout << "=== 圆形 + 像素渲染 ===" << std::endl;
    rasterCircle.Draw();

    // 矩形也一样
    Rectangle rasterRect(raster, 3, 4);
    std::cout << "=== 矩形 + 像素渲染 ===" << std::endl;
    rasterRect.Draw();

    // 缩放是形状自己的操作，不影响渲染器
    rasterRect.Scale(2.0f);
    std::cout << "=== 缩放后的矩形 ===" << std::endl;
    rasterRect.Draw();

    return 0;
}
