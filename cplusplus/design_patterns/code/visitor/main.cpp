#include "visitor.hpp"

int main()
{
    std::vector<std::unique_ptr<Shape>> shapes;

    shapes.push_back(std::make_unique<Circle>(5));
    shapes.push_back(std::make_unique<Rectangle>(3, 4));

    AreaVisitor area;

    for (const auto &shape : shapes)
    {
        shape->Accept(area);
        std::cout << "area = " << area.GetTotalArea() << std::endl;
    }

    std::cout << "total area = " << area.GetTotalArea() << '\n';

    SVGVisitor svg;

    for (const auto &shape : shapes)
    {
        shape->Accept(svg);
    }

    return 0;
}
