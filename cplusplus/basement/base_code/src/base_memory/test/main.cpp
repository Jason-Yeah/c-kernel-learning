#include "resource_manager.h"
#include "graph_node.h"

int main()
{
    resource_manager rm(10);
    std::cout << rm.get_value() << std::endl;
    rm.set_value(20);
    std::cout << rm.get_value() << std::endl;

    resource_manager rm2(100);
    rm = std::move(rm2);

    std::cout << rm.get_value() << std::endl;

    std::cout << "=====================" << std::endl;

    auto n1 = std::make_shared<graph_node>(1);
    auto n2 = std::make_shared<graph_node>(2);
    auto n3 = std::make_shared<graph_node>(3);

    n1->add_edge(n2);
    n2->add_edge(n3);
    n1->add_edge(n3);

    n1->print_edges();
    n2->print_edges();
    n3->print_edges();

    n2.reset();
    n3->add_edge(n1);

    std::cout << "=====================" << std::endl;

    n1->print_edges();
    // n2->print_edges();
    n3->print_edges();

    return 0;
}