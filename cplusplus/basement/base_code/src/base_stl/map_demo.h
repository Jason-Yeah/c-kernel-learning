#ifndef MAP_DEMO_H
#define MAP_DEMO_H

#include <iostream>
#include <map>
#include <string>
#include <algorithm>
#include <functional>

#include <utility> // std::pair
#include <exception> // std::out_of_range
#include <stack>
#include <vector>

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

                tree_node<K, V>& maximun(tree_node<K, V>* node) const
                {
                    if (node == nullptr)
                        throw std::out_of_range("Tree is empty");
                    while (node->_right)
                        node = node->_right;
                    return *node;
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
                V& operator[](const K& key)
                {
                    auto* node = find(key);
                    if (node)
                        return node->_data.second;
                    // 不存在则插入默认值
                    insert({key, V()});
                    node = find(key);
                    return node->_data.second;
                }

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

                class iterator
                {
                    private:
                        tree_node<K, V>* _curr;

                        tree_node<K, V>* minium(tree_node<K, V>* node) const
                        {
                            if (node == nullptr)
                                return nullptr;
                            while (node->_left)
                                node = node->_left;
                            return node;
                        }

                        tree_node<K, V>& maximun(tree_node<K, V>* node) const
                        {
                            if (node == nullptr)
                                throw std::out_of_range("Tree is empty");
                            while (node->_right)
                                node = node->_right;
                            return *node;
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
                        using iterator_category = std::bidirectional_iterator_tag;
                        using value_type = std::pair<K, V>;
                        using difference_type = std::ptrdiff_t;
                        using pointer = std::pair<K, V>*;
                        using reference = std::pair<K, V>&;
                        iterator(tree_node<K, V>* node): _curr(node) {}
                        reference operator*() const { return _curr->_data; }
                        pointer operator->() const { return &(_curr->_data); }
                        reference operator++()
                        {
                            _curr = successor(_curr);
                            return _curr->_data;
                        }
                        iterator operator++(int)
                        {
                            auto temp = *this;
                            ++ *this; // _curr = successor(_curr);
                            return temp;
                        }
                        // 省略operator--()实现
                        bool operator==(const iterator& other) const { return _curr == other._curr; }
                        bool operator!=(const iterator& other) const { return _curr != other._curr; }                        
                };

                iterator begin() const
                {
                    return iterator(minium(_root)); // 最小节点即为迭代器的起点
                }
                iterator end() const
                {
                    return iterator(nullptr);
                }
        };
    };
    namespace avl_map
    {
        template<typename K, typename V>
        struct tree_node
        {
            K _key;
            V _value;
            int _height;
            tree_node* _left;
            tree_node* _right;
            tree_node(const K& k, const V& v) : _key(k), _value(v), _height(1), _left(nullptr), _right(nullptr) {}
        };

        template<typename K, typename V>
        class map
        {
            private:
                tree_node<K, V>* _root;
                int get_height(tree_node<K, V>* node) const
                {
                    return node ? node->_height : 0;
                }
                int get_balance(tree_node<K, V>* node) const // 左-右
                {
                    return node ? get_height(node->_left) - get_height(node->_right) : 0;
                }

                // 调整树的平衡
                tree_node<K, V>* right_rotate(tree_node<K, V>* y)
                {
                    auto* x = y->_left;
                    auto* t2 = x->_right;
                    x->_right = y;
                    y->_left = t2;
                    y->_height = 1 + std::max(get_height(y->_left), get_height(y->_right));
                    x->_height = 1 + std::max(get_height(x->_left), get_height(x->_right));
                    return x;
                }
                tree_node<K, V>* left_rotate(tree_node<K, V>* y)
                {
                    auto* x = y->_right;
                    auto* t2 = x->_left;
                    x->_left = y;
                    y->_right = t2;
                    y->_height = 1 + std::max(get_height(y->_left), get_height(y->_right));
                    x->_height = 1 + std::max(get_height(x->_left), get_height(x->_right));
                    return x;
                }

                tree_node<K, V>* get_min(tree_node<K, V>* node)
                {
                    if (node == nullptr) return nullptr;
                    auto* curr = node;
                    while (curr->_left)
                        curr = curr->_left;
                    return curr;
                }

                                tree_node<K, V>* insert(tree_node<K, V>* node,
                     const K& key, const V& value)
                {
                    if (node == nullptr)
                        return new tree_node<K, V>(key, value);
                    if (key < node->_key) node->_left = insert(node->_left, key, value);
                    else if (key > node->_key) node->_right = insert(node->_right, key, value);
                    else
                    {
                        node->_value = value; // 更新值
                        return node;
                    }

                    node->_height = 1 + std::max(get_height(node->_left), get_height(node->_right));
                    int balance = get_balance(node);

                    // LL
                    if (balance > 1 && key < node->_left->_key)
                        return right_rotate(node);
                    // RR
                    if (balance < -1 && key > node->_right->_key)
                        return left_rotate(node);
                    // LR
                    if (balance > 1 && key > node->_left->_key)
                    {
                        node->_left = left_rotate(node->_left);
                        return right_rotate(node);
                    }
                    // RL
                    if (balance < -1 && key < node->_right->_key)
                    {
                        node->_right = right_rotate(node->_right);
                        return left_rotate(node);
                    }

                    // 平衡就返回
                    return node;
                }

                tree_node<K, V>* erase(tree_node<K, V>* root, const K& key)
                {
                    if (root == nullptr) return nullptr;
                    if (key < root->_key) 
                        root->_left = erase(root->_left, key);
                    else if (key > root->_key)
                        root->_right = erase(root->_right, key);
                    else
                    {
                        // 1个或无节点情况
                        if ((root->_left == nullptr) || (root->_right == nullptr))
                        {
                            auto* tmp = root->_left ? root->_left : root->_right;
                            if (tmp == nullptr)
                            {
                                delete root;
                                return nullptr;
                            }
                            else
                            {
                                auto* to_del = root;
                                root = tmp;
                                delete to_del;
                            }
                        }
                        else // 两个节点
                        {
                            auto* succ = get_min(root->_right);
                            root->_key = succ->_key;
                            root->_value = succ->_value;
                            root->_right = erase (root->_right, succ->_key);
                        }
                    }

                    if (root == nullptr) return root;
                    root->_height = 1 + std::max(get_height(root->_left), get_height(root->_right));
                    int balance = get_balance(root);
                    if (balance > 1 && get_balance(root->_left) >= 0) return right_rotate(root);
                    if (balance > 1 && get_balance(root->_left) < 0) 
                    {
                        root->_left = left_rotate(root->_left);
                        return right_rotate(root);
                    }
                    if (balance < -1 && get_balance(root->_right) < 0) return left_rotate(root);
                    if (balance < -1 && get_balance(root->_right) >= 0)
                    {
                        root->_right = right_rotate(root->_right);
                        return left_rotate(root);
                    }

                    return root;
                }

                V* find(tree_node<K, V>* node, const K& key) const
                {
                    if (node == nullptr) return nullptr;
                    if (key == node->_key) return &(node->_value);
                    else if (key < node->_key) return find(node->_left, key);
                    else return find(node->_right, key);
                }

                void inorder_helper(tree_node<K, V>* node, std::vector<std::pair<K, V>>& res) const 
                {
                    if (node == nullptr) return ;
                    inorder_helper(node->_left, res);
                    res.push_back({node->_key, node->_value});
                    inorder_helper(node->_right, res);
                }

            public:
                map(): _root(nullptr) {}

                ~map()
                {
                    std::function<void(tree_node<K, V>*)> destory = [&](tree_node<K, V>* node) {
                        if (node != nullptr)
                        {
                            destory(node->_left);
                            destory(node->_right);
                            delete node;
                        }
                    };
                    destory(_root);
                }

                void put(const K& key, const V& val)
                {
                    _root = insert(_root, key, val);
                }

                V* get(const K& key)
                {
                    return find(_root, key);
                }

                void remove(const K& key)
                {
                    _root = erase(_root, key);
                }

                std::vector<std::pair<K, V>> inorder_traversal() const
                {
                    std::vector<std::pair<K, V>> res;
                    inorder_helper(_root, res);
                    return res;
                }

                tree_node<K, V>* get_root() const
                {
                    return _root;
                }
        };
    };
};

#endif // MAP_DEMO_H
