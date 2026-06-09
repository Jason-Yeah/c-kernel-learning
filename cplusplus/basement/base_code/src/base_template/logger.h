
#ifndef LOGGER_H
#define LOGGER_H

#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>

namespace sen_std {
    /** 
    * Gereral Template
    */
    template <typename T, typename Enable = void>
    class logger
    {
        public:
            static void log(const T& val)
            {
                std::cout << "General logger: " << val << std::endl;
            }
    };

    /**
    * Partial Specialization
    */
    template<typename T>
    class logger<T, std::enable_if_t<std::is_pointer_v<T>>>
    {
        public:
            static void log(const T& val)
            {
                std::cout << "Pointer logger: " << *val << std::endl;
            }
    };

    /**
    * Full Specialization
    */
    template<>
    class logger<std::string>
    {
        public:
            static void log(const std::string& val)
            {
                std::cout << "String logger: " << val << std::endl;
            }
    };

    // Function template for recursiving call.
    template<typename T>
    void log_one(const T& val)
    {
        logger<T>::log(val);
    }

    template<typename... Args>
    void log_all(const Args&... args)
    {
        (log_one(args), ...);
    }

    template<int N>
    struct factorial
    {
        inline static constexpr int val = N * factorial<N - 1>::val;
    };

    template<>
    struct factorial<0>
    {
        inline static constexpr int val = 1;
    };

    typedef long long LL;

    template<int N>
    struct fibonacci
    {
        inline static const LL val = fibonacci<N - 1>::val + fibonacci<N - 2>::val;
    };

    template<>
    struct fibonacci<0>
    {
        inline static const LL val = 0;
    };

    template<>
    struct fibonacci<1>
    {
        inline static const LL val = 1;
    };

    // std::declval在不实际创建T类型对象情况下，生成一个T类型右值引用
    // std::false_type中有一个静态常量`value`默认为false。
    template<typename T, typename = void>
    struct is_addable: std::false_type {};

    template<typename T>
    struct is_addable<T, decltype(void(std::declval<T>() + std::declval<T>()))>
            : std::true_type {} ;

    template<int... Ns>
    struct sum;

    template<>
    struct sum<>
    {
        static const int val = 0;
    };

    template<int N, int... Ns>
    struct sum<N, Ns...>{
        static const int val = N + sum<Ns...>::val;
    };
    
    template<typename... Ts>
    struct type_list {};

    template<typename _Tlist, std::size_t N>
    struct type_at {};

    template<typename _head, typename ... _tail>
    struct type_at<type_list<_head, _tail...>, 0>
    {
        using type = _head;
    };

    template<typename _head, typename ... _tail, std::size_t N>
    struct type_at<type_list<_head, _tail...>, N>
    {
        using type = typename type_at<type_list<_tail...>, N-1>::type;
    };

}; 

#endif  // LOGGER_H
