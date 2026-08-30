#pragma once

#include <iostream>
#include <memory>
#include <vector>

class Circle;
class Rectangle;

class ShapeVisitor
{
public:
    virtual ~ShapeVisitor() = default;

    virtual void Visit(const Circle &circle) = 0;
    virtual void Visit(const Rectangle &rect) = 0;
};

class Shape
{
public:
    virtual ~Shape() = default;

    virtual void Accept(ShapeVisitor &visitor) const = 0;
};

class Circle final : public Shape
{
    double radius_;

public:
    explicit Circle(double radius) : radius_(radius) {}

    double GetRadius() const { return radius_; }

    void Accept(ShapeVisitor &visitor) const override
    {
        // 第一次动态分派已经确定：
        //
        // Shape::Accept(...)
        //        ↓
        // Circle::Accept(...)
        //
        // 此时 *this 的静态类型就是 const Circle&。
        //
        // 编译器因此选择 Visit(const Circle&) 这个重载，
        // 随后再根据 visitor 的实际类型进行第二次虚分派。
        visitor.Visit(*this);
    }
};

class Rectangle final : public Shape
{
    double width_;
    double height_;

public:
    Rectangle(double width, double height) : width_(width), height_(height) {}

    double GetWidth() const { return width_; }

    double GetHeight() const { return height_; }

    void Accept(ShapeVisitor &visitor) const override { visitor.Visit(*this); }
};

class AreaVisitor final : public ShapeVisitor
{
    double total_ = 0.0;

public:
    void Visit(const Circle &circle) override
    {
        total_ += 3.1415926 * circle.GetRadius() * circle.GetRadius();
    }

    void Visit(const Rectangle &rectangle) override
    {
        total_ += rectangle.GetWidth() * rectangle.GetHeight();
    }

    double GetTotalArea() const { return total_; }
};

class SVGVisitor final : public ShapeVisitor
{
public:
    void Visit(const Circle &circle) override
    {
        std::cout << "<circle r=\"" << circle.GetRadius() << "\" />\n";
    }

    void Visit(const Rectangle &rectangle) override
    {
        std::cout << "<rect width=\"" << rectangle.GetWidth() << "\" height=\""
                  << rectangle.GetHeight() << "\" />\n";
    }
};