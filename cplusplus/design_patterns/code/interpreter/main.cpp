#include "interpreter.hpp"

int main()
{
    Context context;

    context.SetVariable("x", 3); // x = 3

    auto tree = std::make_unique<MultiplyExpression>(
        std::make_unique<VariableExpression>("x"),
        std::make_unique<AddExpression>(std::make_unique<NumberExpression>(4),
                                        std::make_unique<NumberExpression>(5)));

    int res = tree->Interpret(context);
    std::cout << "x * (4 + 5) = " << res << "  (x = 3)" << std::endl;

    Context context2;
    context2.SetVariable("x", 10);
    std::cout << "换 x = 10 再解释: " << tree->Interpret(context2) << std::endl;

    

    return 0;
}
