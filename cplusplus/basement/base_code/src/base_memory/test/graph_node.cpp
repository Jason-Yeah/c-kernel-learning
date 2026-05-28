#include "graph_node.h"

graph_node::graph_node(int id): _id(id) {}

void graph_node::add_edge(const std::shared_ptr<graph_node>& node)
{
    if (!node || node.get() == this) return ;
    _vec_gnode.push_back(node);
}

void graph_node::print_edges() const
{
    std::cout << "The node " << _id << " is connected to: ";
    for (const auto& node: _vec_gnode)
    {
        if (auto locked_node = node.lock())
            std::cout << locked_node->_id << " ";
        else
        {
            std::cout << "[fail_node]";
            break;
        }
    } 
    std::cout << std::endl;
}