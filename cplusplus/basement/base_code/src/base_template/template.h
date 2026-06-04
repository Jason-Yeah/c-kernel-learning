#ifndef TEMPLATE_DEMO_H
#define TEMPLATE_DEMO_H

#include <iostream>
#include <string>
#include <vector>
#include <list>

template <typename T>
T max_val(T a, T b)
{
    return a > b ? a : b;
}

namespace sen_std
{
    template <typename T, typename U>
    class pair
    {
        private:
            T first;
            U second;
        public:
            pair(T a, U b): first(a), second(b) {}

            void print() const
            {
                std::cout << "pair: " << first << ", " << second << std::endl;
            }
    };

    template <typename T, size_t N>
    class fixed_array
    {
        private:
            T _arr[N];
        public:
            T& operator[](std::size_t index)
            {
                return _arr[index];
            }

            void print() const
            {
                for (size_t i = 0; i < N; ++ i )
                    std:: cout << _arr[i] << ' ';
                std::cout << std::endl;
            }
    };

    template <template <typename, typename> class container, typename T>
    class container_printer
    {
        public:
            void print(const container<T, std::allocator<T>>& _container )
            {
                for (const auto& elem: _container)
                    std::cout << elem << " ";
                std::cout << std::endl;
            }
    };
};

#endif  // TEMPLATE_DEMO_H