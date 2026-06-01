#ifndef MAP_DEMO_H
#define MAP_DEMO_H

#include <iostream>
#include <map>
#include <string>
#include <algorithm>

#include <utility> // std::pair
#include <exception> // std::out_of_range
#include <stack>

namespace sen_std
{
    namespace bst_map
    {
        template<typename K, typename V>
        struct tree_node
        {
            std::pair<K, V> _data;
            tree_node* _left;
            tree_node* _right;
            tree_node* _parent;
            tree_node(const K& key, const V& val, tree_node* parent = nullptr)
                : _data({key, val})
                , _left(nullptr)
                , _right(nullptr)
                , _parent(parent) {}
        };

        template<typename K, typename V>
        class map
        {
            private:
                tree_node<K, V>* _root;

                void clear(tree_node<K, V>* node)
                {
                    // dfs
                    if (node == nullptr)
                        return;
                    clear(node->_left);
                    clear(node->_right);
                    delete node;
                }

                tree_node<K, V>* minium(tree_node<K, V>* node) const
                {
                    if (node == nullptr)
                        return nullptr;
                    while (node->_left)
                        node = node->_left;
                    return node;
                }

                tree_node<K, V>* successor(tree_node<K, V>* node) const
                {
                    if (node->_right != nullptr)
                        return minium(node->_right);
                    auto* p = node->_parent;
                    while (p != nullptr && node == p->_right)
                    {
                        node = p;
                        p = p->_parent;
                    }
                    return p;
                }
            public:
                map(): _root(nullptr) {}
                void clear()
                {
                    clear(_root);
                    _root = nullptr;
                }
                ~map()
                {
                    clear();
                }
                map(const map& other) = delete;
                map& operator=(const map& other) = delete;

                void insert(const std::pair<K, V>& data)
                {
                    if (_root == nullptr)
                    {
                        _root = new tree_node<K, V>(data.first, data.second);
                        return ;
                    }
                    tree_node<K, V>* curr = _root;
                    tree_node<K, V>* parent = nullptr;
                    while (curr)
                    {
                        parent = curr;
                        if (data.first < curr->_data.first) curr = curr->_left;
                        else if (data.first > curr->_data.first) curr = curr->_right;
                        else
                        {
                            // 有key则覆盖
                            curr->_data.second = data.second;
                            return ;
                        }
                    }

                    if(data.first < parent->_data.first)
                        parent->_left = new tree_node<K, V>(data.first, data.second, parent);
                    else
                        parent->_right = new tree_node<K, V>(data.first, data.second, parent);
                }

                tree_node<K, V>* find(const K& key) const
                {
                    auto* curr = _root;
                    while (curr)
                        if (key < curr->_data.first) curr = curr->_left;
                        else if (key > curr->_data.first) curr = curr->_right;
                        else return curr;
                    return nullptr;
                }

                void erase(const K& key)
                {
                    auto* node = find(key);
                    if (node == nullptr) return ;
                    if (node->_left && node->_right)
                    {
                        auto* succ = successor(node);
                        node->_data = std::move(succ->_data);
                        node = succ;
                    }

                    auto* child = node->_left ? node->_left : node->_right;
                    if (child != nullptr)
                        child->_parent = node->_parent;

                    if (node->_parent == nullptr)
                    {
                        _root = child;
                    }
                    else if (node == node->_parent->_left)
                    {
                        node->_parent->_left = child;
                    }
                    else
                    {
                        node->_parent->_right = child;
                    }
                    delete node;
                }
        };
    };
};

#endif // MAP_DEMO_H
