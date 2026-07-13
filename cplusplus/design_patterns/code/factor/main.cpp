
#include "operation_factory.hpp"
#include <iostream>

int main()
{
    // OperatorFactory factory;
    // auto op = factory.createOperate("+");
    // op->_num_a = 1;
    // op->_num_b = 2;

    // auto res = op->get_res();
    // std::cout << res << std::endl;

    // auto sub = factory.createOperate("-");
    // sub->_num_a = 10;
    // sub->_num_b = 5;
    // std::cout << "10 - 5 = " << sub->get_res() << std::endl;

    auto add = OperatorFactory::createOperate("+");
    add->_num_a = 1;
    add->_num_b = 2;
    std::cout << add->get_res() << std::endl;
    return 0;
}
