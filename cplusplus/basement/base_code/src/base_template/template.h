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

    template<typename T>
    class printer
    {
        public:
            void print(const T& o)
            {
                std::cout << "g: " << o << std::endl;
            }
    };

    template<>
    class printer<std::string>
    {
        public:
            void print(std::string o)
            {
                std::cout << "s: " << o << std::endl;
            }
    };

    template <typename T, typename U>
    class pair<T, U*>
    {
        private:
            T first;
            U* second;
        public:
            pair(T a, U* b): first(a), second(b) {}

            void print() const
            {
                std::cout << "pair: " << first << ", " << *second << std::endl;
            }
    };

    template<typename T>
    void printval(const T& val)
    {
        std::cout << "General print: " << val << std::endl;
    }

    template<>
    void printval<std::string>(const std::string& val)
    {
        std::cout << "String print: " << val << std::endl;
    }

    template<>
    void printval<int*>(int* const& val)
    {
        std::cout << "int* : " << *val << std::endl;
    }
    // 展开终止条件
    void printall()
    {
        std::cout << std::endl;
    }

    template <typename T, typename... Args>
    void printall(const T& first, const Args&... args)
    {
        std::cout << first << " ";
        printall(args...);
        
    }

    template <typename... Args>
    void coutall(const Args&... args)
    {
        ((std::cout << args << " "), ...);
        std::cout << std::endl;
    }

    template<typename... Args>
    auto sum_all(Args... args)-> decltype((args + ... ))
    {
        return (args + ...);
    }

    template<typename... Args>
    bool all_not(const Args&... args)
    {
        return (!args && ...);
    } 

    template<typename... Args>
    auto sum_left_fold(const Args&... args) -> decltype((args + ...))
    {
        return (args + ...);
    }

    template<typename... Args>
    auto multi_right_fold(const Args&... args) -> decltype((args * ...))
    {
        return (... * args);
    }

    template<typename... Args>
    auto print_all(const Args&... args)
    {
        (std::cout << ... << args) << std::endl;
    }

    struct my_point
    {
        int x, y;
        my_point(int a, int b): x(a), y(b) {}
        my_point operator+(const my_point& o) const
        {
            return my_point(x + o.x, y + o.y);
        }
    };

    template<typename... Args>
    auto sum_point(const Args... args) -> decltype((args + ...))
    {
        return (args + ...);
    }

};

#endif  // TEMPLATE_DEMO_H