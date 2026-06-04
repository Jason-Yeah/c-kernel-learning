#ifndef UMAP_DEMO_H
#define UMAP_DEMO_H

#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>
#include <exception>
#include <functional>
#include <list>
#include <cmath>
#include <iterator>

namespace sen_std
{
    template<typename K, typename V>
    struct hash_node
    {
        std::pair<const K, V> _data;
        hash_node<K, V>* _next;
        hash_node(const K& key, const V& val): _data(key, val), _next(nullptr) {}
    };

    template<typename K, typename V, typename Hash = std::hash<K>>
    class unordered_map
    {
        public:
            class iterator;
            using key_type = K;
            using mapped_type = V;
            using value_type = std::pair<const K, V>;
            using size_type = std::size_t;

            unordered_map(size_type init_capacity = 8, double max_load_factor = 1.00)
                        : _bucket_count(init_capacity), _max_load_factor(max_load_factor), _elem_count(0),
                          _hash_func(Hash())
                        {
                            _buckets.resize(_bucket_count);
                        }
            ~unordered_map()
            {
                clear();
            }
            
            unordered_map(const unordered_map& other) = delete;
            unordered_map& operator=(const unordered_map& other) = delete;

            void insert(const K& key, const V& value)
            {
                size_type hash_value = _hash_func(key);
                size_type idx = hash_value % _bucket_count;

                auto* node = _buckets[idx];
                while (node)
                {
                    if (node->_data.first == key)
                    {
                        node->_data.second = value;
                        return ;
                    }
                    node = node->_next;
                }
                auto* new_node = new hash_node<K, V>(key, value);
                new_node->_next = _buckets[idx];
                _buckets[idx] = new_node;
                ++ _elem_count;
                
                double load_factor = static_cast<double> (_elem_count) / _bucket_count;
                if (load_factor > _max_load_factor)
                    rehash(_bucket_count * 2);
            }

            V* find(const K& key)
            {
                auto hash_val = _hash_func(key);
                auto idx = hash_val % _bucket_count;
                auto* node = _buckets[idx];
                while (node)
                {
                    if (node->_data.first == key)
                        return &(node->_data.second);
                    node = node->_next;
                }
                return nullptr;
            }

            bool erase(const K& key)
            {
                auto hash_val = _hash_func(key);
                auto idx = hash_val % _bucket_count;
                auto* node = _buckets[idx];
                hash_node<K, V>* prev = nullptr;

                while (node)
                {
                    if (node->_data.first == key)
                    {
                        if (prev == nullptr)
                            _buckets[idx] = node->_next;
                        else
                            prev->_next = node->_next;
                        delete node;
                        -- _elem_count;
                        
                        if (_elem_count > 0) // 至少一个
                        {
                            double load_factor = static_cast<double> (_elem_count) / _bucket_count;
                            size_type new_count = std::max<size_type>(_bucket_count / 2, _MIN_BUCKETS);
                            if (load_factor < (_max_load_factor / 4.0) && new_count < _bucket_count)
                                rehash(new_count);
                        }
                        return true;
                    }
                    prev = node;
                    node = node->_next;
                }
                return false;
            }

            size_type size() const
            {
                return _elem_count;
            }

            bool empty() const
            {
                return _elem_count == 0;
            }

            void clear()
            {
                for (size_t i = 0; i < _buckets.size(); ++ i)
                {
                    hash_node<K, V>* curr = _buckets[i];
                    while (curr)
                    {
                        auto* t = curr;
                        curr = curr->_next;
                        delete t;
                    }
                    _buckets[i] = nullptr;
                }
                _elem_count = 0;

                if (_bucket_count > _MIN_BUCKETS)
                    rehash(_MIN_BUCKETS);
            }

            iterator begin()
            {
                for (size_type i = 0; i < _bucket_count; ++ i)
                    if (_buckets[i]) return iterator(this, i, _buckets[i]);

                return end();
            }

            iterator end()
            {
                return iterator(this, _bucket_count, nullptr);
            }

            class iterator
            {
                public:
                    using iterator_cayegory = std::forward_iterator_tag;
                    using value_type = std::pair<const K, V>;
                    using difference_type = std::ptrdiff_t;
                    using point = value_type*;
                    using reference = value_type&;

                    iterator(unordered_map* map, size_type bucket_index, hash_node<K, V>* node)
                            : _map(map), _bucket_index(bucket_index), _curr(node) {}

                    reference operator*() const
                    {
                        if (_curr == nullptr) throw std::out_of_range("Iterator out of range");
                        return _curr->_data;
                    }
                    point operator->() const
                    {
                        if (_curr == nullptr) throw std::out_of_range("Iterator out of range");
                            // return nullptr;
                        return &(_curr->_data);
                    }

                    iterator& operator++()
                    {
                        advance();
                        return *this;
                    }
                    iterator operator++(int)
                    {
                        iterator tmp = *this;
                        advance();
                        return tmp;
                    }

                    bool operator==(const iterator& other) const
                    {
                        return _map == other._map && _bucket_index == other._bucket_index
                                        && _curr == other._curr;
                    }
                    bool operator!=(const iterator& other) const
                    {
                        return !(*this == other);
                    }

                private:
                    unordered_map* _map;
                    size_type _bucket_index;
                    hash_node<K, V>* _curr;
                    void advance()
                    {
                        if (_curr)
                            _curr = _curr->_next;
                        while (_bucket_index + 1 < _map->_bucket_count && _curr == nullptr)
                        {
                            ++ _bucket_index;
                            _curr = _map->_buckets[_bucket_index];
                        }
                        if (_curr == nullptr)
                            _bucket_index = _map->_bucket_count;
                    }
            };

        private:
            std::vector<hash_node<K, V>*> _buckets;
            size_type _bucket_count;
            size_type _elem_count;
            double _max_load_factor;
            Hash _hash_func;

            size_type _MIN_BUCKETS = 8;

            void rehash(const size_type& new_bucket_count)
            {
                if (new_bucket_count == 0 || new_bucket_count == _bucket_count) return ;

                std::vector<hash_node<K, V>*> new_buckets(new_bucket_count, nullptr);
                for (size_type i = 0; i < _bucket_count; ++ i)
                {
                    auto* node = _buckets[i];
                    while (node)
                    {
                        auto* next_node = node->_next;
                        auto new_idx = _hash_func(node->_data.first) % new_bucket_count;
                        node->_next = new_buckets[new_idx];
                        new_buckets[new_idx] = node;
                        node = next_node;
                    }
                }

                _buckets = std::move(new_buckets);
                _bucket_count = new_bucket_count;
            }
    };
};

#endif // UMAP_DEMO_H
