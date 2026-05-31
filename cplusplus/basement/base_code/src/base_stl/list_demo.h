#ifndef LIST_DEMO_H
#define LIST_DEMO_H

#include <iostream>

namespace sen_stl
{
    template <typename T>
    struct node
    {
        T _data;
        node<T>* _next;
        node<T>* _prev;
        node(const T& data = T()): _data(data), _next(nullptr), _prev(nullptr) {}
    };

    // std::list<T>::iterator it iterator属于list<T>类的嵌套类型
    template <typename T>
    class list;

    template <typename T>
    class __list_iterator
    {
        public:
            using self_type = __list_iterator<T>;
            using value_type = T;
            using pointer = T*;
            using reference = T&;
            using iterator_category = std::bidirectional_iterator_tag;
            using difference_type = std::ptrdiff_t;

            // 通过构造传递没有new开辟，暂时不考虑析构
            __list_iterator(node<T>* node_p = nullptr): _node_p(node_p) {}

            reference operator*() { return _node_p->_data;}
            pointer operator-> () { return &(_node_p->_data); }

            self_type& operator++() // ++ it;
            {
                if(_node_p)  _node_p = _node_p->_next;
                return *this;
            }
            self_type operator++(int) // it ++;
            {
                self_type temp = *this;
                // if(node_p)  _node_p = _node_p->_next;
                ++ (*this);
                return temp;
            } // 所以遍历多选择 ++it; 因为++it; 只需要一次迭代器的复制，而it++; 需要两次迭代器的复制

            self_type& operator--() // -- it;
            {
                if(_node_p)  _node_p = _node_p->_prev;
                return *this;
            }
            self_type operator--(int) // it --;
            {
                self_type temp = *this;
                // if(node_p)  _node_p = _node_p->_prev;
                -- (*this);
                return temp;
            }
            
            bool operator==(const self_type& other) const {return _node_p == other._node_p;}
            bool operator!=(const self_type& other) const {return !(*this == other);}
        private:
            node<T>* _node_p;
            friend class list<T>;
    };

    template <typename T>
    class list
    {
        public:
            using iterator = __list_iterator<T>;
            using const_iterator = __list_iterator<T>;
            list()
            {
                _head = new node<T>();
                _tail = new node<T>();
                _head->_next = _tail;
                _tail->_prev = _head;
            }
            ~list()
            {
                clear();
                delete _head;
                delete _tail;
            }
            // 禁止拷贝构造和赋值操作
            list(const list& other) = delete;
            list& operator=(const list& other) = delete;

            iterator insert(iterator pos, const T& value)
            {
                node<T>* new_n = new node<T>(value);
                new_n->_next = pos._node_p;
                new_n->_prev = pos._node_p->_prev;
                pos._node_p->_prev->_next = new_n;
                pos._node_p->_prev = new_n;
                return iterator(new_n);
            }
            
            iterator erase(iterator pos)
            {
                node<T>* to_delete = pos._node_p;
                if (to_delete == _head || to_delete == _tail) return iterator(_tail); // 不能删除头尾节点
                to_delete->_prev->_next = to_delete->_next;
                to_delete->_next->_prev = to_delete->_prev;
                iterator next_it(to_delete->_next);
                delete to_delete;
                return next_it;
            }

            void push_front(const T& value)
            {
                insert(begin(), value);
            }

            void push_back(const T& value)
            {
                insert(end(), value);
            }

            void pop_front()
            {
                if (!empty()) erase(begin());
            }

            void pop_back()
            {
                if (!empty()) erase(iterator(_tail->_prev));
            }

            bool empty() const { return _head->_next == _tail; }
            // bool empty() const { return begin() == end(); }

            iterator begin() { return iterator(_head->_next); }
            iterator end() { return iterator(_tail); }

            T& front() { return _head->_next->_data; }
            T& back() { return _tail->_prev->_data; }

            size_t size() const
            {
                size_t count = 0;
                for (auto it = begin(); it != end(); ++ it)
                    ++count;
                return count;
            }

            void print() const
            {
                node<T>* current = _head->_next;
                while (current != _tail)
                {
                    std::cout << current->_data << " ";
                    current = current->_next;
                }
                std::cout << std::endl;
            }

            void clear()
            {
                node<T>* current = _head->_next;
                while (current != _tail)
                {
                    auto next = current->_next;
                    delete current;
                    current = next;
                }
                _head->_next = _tail;
                _tail->_prev = _head;
            }

            void remove(const T& value)
            {
                for (auto it = begin(); it != end(); )
                {
                    if (*it == value)
                        it = erase(it);
                    else
                        ++it;
                }
            }
        private:
            node<T>* _head;
            node<T>* _tail;
    };
}
#endif // LIST_DEMO_H