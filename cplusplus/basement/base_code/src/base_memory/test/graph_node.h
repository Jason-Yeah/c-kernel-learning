#ifndef _TEST_GRAPH_NODE_H
#define _TEST_GRAPH_NODE_H

#include <iostream>
#include <memory>
#include <vector>

class graph_node
{
    private:
        int _id;
        std::vector<std::weak_ptr<graph_node>> _vec_gnode;
    public:
        graph_node() = default;
        graph_node(int id);
        ~graph_node() = default;

        void add_edge(const std::shared_ptr<graph_node>& node);
        void print_edges() const;
};

#endif // _TEST_GRAPH_NODE_H