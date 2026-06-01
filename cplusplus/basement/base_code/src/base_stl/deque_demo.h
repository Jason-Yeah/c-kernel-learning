#ifndef DEQUE_DEMO_H
#define DEQUE_DEMO_H

#include <iostream>
#include <deque>

namespace sen_std
{
    template <typename T>
    class deque
    {
        private:
            T* _buffer;
            size_t _capacity;
            size_t _count;
            size_t _front_index;
            size_t _back_index;
        public:
            deque(size_t initial_capacity = 10): _capacity(initial_capacity), _count(0), _front_index(0), _back_index(0)
            {
                _buffer = new T[_capacity]; // 使用new会调用默认构造函数
            }

            ~deque()
            {
                delete[] _buffer;
            }

            bool empty() const
            {
                return _count == 0;
            }

            size_t size() const
            {
                return _count;
            }

            void resize(size_t new_capacity)
            {
                T* new_buf = new T[new_capacity];
                for (size_t i = 0; i < _count; ++ i)
                    new_buf[i] = _buffer[(_front_index + i) % _capacity];
                
                    _front_index = 0;
                _back_index = _count;
                delete[] _buffer; // 防止内存泄露
                _buffer = new_buf;
                _capacity = new_capacity;
            }

            void push_front(const T& value)
            {
                if (_front_index == _back_index || _count == _capacity)
                    resize(_capacity * 2);
                _front_index = (_front_index - 1 + _capacity) % _capacity;
                _buffer[_front_index] = value;
                ++ _count;
            }

            void push_back(const T& value)
            {
                if (_count == _capacity)
                    resize(_capacity * 2);
                _buffer[_back_index] = value;
                _back_index = (_back_index + 1) % _capacity;
                ++ _count;
            }

            void pop_front()
            {
                if (empty())
                    throw std::out_of_range("Deque is empty");
                _front_index = (_front_index + 1) % _capacity;
                -- _count;
            }

            void pop_back()
            {
                if (empty())
                    throw std::out_of_range("Deque is empty");
                _back_index = (_back_index - 1 + _capacity) % _capacity;
                -- _count;
            }

            T& front()
            {
                if (empty())
                    throw std::out_of_range("Deque is empty");
                return _buffer[_front_index];
            }

            const T& front() const
            {
                if (empty())
                    throw std::out_of_range("Deque is empty");
                return _buffer[_front_index];
            }

            T& back()
            {
                if (empty())
                    throw std::out_of_range("Deque is empty");
                return _buffer[(_back_index - 1 + _capacity) % _capacity];
            }

            const T& back() const
            {
                if (empty())
                    throw std::out_of_range("Deque is empty");
                return _buffer[(_back_index - 1 + _capacity) % _capacity];
            }

            class iterator
            {
                private:
                    deque<T>* _deque;
                    size_t _pos;
                public:
                    using iterator_category = std::bidirectional_iterator_tag;
                    using value_type = T;
                    using difference_type = std::ptrdiff_t;
                    using pointer = T*;
                    using reference = T&;

                    iterator(deque<T>* d, size_t pos): _deque(d), _pos(pos) {}
                    
                    reference operator*() const
                    {
                        auto real_idx = (_deque->front() + _pos) % _deque->_capacity;
                        return _deque->_buffer[real_idx];
                    }
                    pointer operator->() const
                    {
                        size_t real_idx = _deque->front + _pos % _deque->_capacity;
                        return &(_deque->_buffer[real_idx]);
                    }

                    iterator& operator++ ()
                    {
                        ++ _pos;
                        return *this;
                    }
                    iterator operator++(int)
                    {
                        iterator tmp = *this;
                        ++ (*this); // ++ _pos;
                        return tmp;
                    }

                    iterator& operator-- ()
                    {
                        -- _pos;
                        return *this;
                    }
                    iterator operator--(int)
                    {
                        iterator tmp = *this;
                        -- (*this); // -- _pos;
                        return tmp;
                    }

                    bool operator==(const iterator& other) const
                    {
                        return _deque == other._deque && _pos == other._pos;
                    }
                    bool operator!=(const iterator& other) const
                    {
                        return !(*this == other);
                    }
            };

            iterator begin() { return iterator(this, 0); }
            iterator end() { return iterator(this, _count); }

    };
};  // namespace sen_std

#endif // DEQUE_DEMO_H