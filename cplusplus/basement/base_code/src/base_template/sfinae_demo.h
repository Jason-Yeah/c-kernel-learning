#ifndef SFINAE_DEMO_H
#define SFINAE_DEMO_H

#include <iostream>
#include <type_traits>
#include <concepts>
#include <vector>

template<typename T>
typename std::enable_if_t<std::is_integral_v<T>, void> 
print_type(T val)
{
    std::cout << "I val: " << val << std::endl;
}

template<typename T>
typename std::enable_if_t<std::is_floating_point_v<T>, void>
print_type(T val)
{
    std::cout << "F val: " << val << std::endl;
}

// C 风格字符串重载：匹配 const char* 和 char*
// template<typename T>
// std::enable_if_t<
//     std::is_pointer_v<T> &&
//     std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>,
//     void>
// print_type(T val)
// {
//     std::cout << "C str: " << val << std::endl;
// }

template <typename  T>
typename std::enable_if_t<std::is_same_v<T, const char*> || 
            std::is_same_v<T, char*>, void>
print_type(T val)
{
    std::cout << "C str: " << val << std::endl;
}

template <typename  T>
typename std::enable_if_t<std::is_pointer_v<T> && !(std::is_same_v<T, char*>)
&& !(std::is_same_v<T, const char*>), void>
print_type(T val)
{
    std::cout << "Pointer str: " << *val << std::endl;
}

template<typename T>
concept integer = std::is_integral_v<T>;

void cout_type(integer auto value)
{
    std::cout << "Int type" << std::endl;
}

template <typename T>
class has_foo
{
    private:
        typedef char yes[1];
        typedef char no[2];
        template<typename U, void(U::*)()>
        struct SFINAE {};

        template<typename U>
        static yes& test(SFINAE<U, &U::foo>*);
        
        template<typename U>
        static no& test(...);
    public:
        static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes);
};

template <typename T>
typename std::enable_if_t<has_foo<T>::value, void>
call_foo(T &o)
{
    o.foo();
    std::cout << "foo() called" << std::endl;
}

class WithFoo {
public:
void foo() { std::cout << "WithFoo::foo()" << std::endl; }
};

class WithoutFoo {};

// 1. Trait：检测 T 是否有非 void 的 value_type
// 是void返回false
template <typename T, typename = void>
struct has_non_void_value_type : std::false_type {};

// 非void传true
template <typename T>
struct has_non_void_value_type<T, std::enable_if_t<!std::is_void_v<typename T::value_type>>>
    : std::true_type {};

// 2. TypePrinter：通过 bool 参数做特化分发（避免 SFINAE 歧义）
template <typename T, bool HasValueType = has_non_void_value_type<T>::value>
struct TypePrinter {};

template <typename T>
struct TypePrinter<T, false> {
    static void print() {
        std::cout << "T has no value_type (or value_type is void)" << std::endl;
    }
};

template <typename T>
struct TypePrinter<T, true> {
    static void print() {
        std::cout << "T has a member type value_type" << std::endl;
    }
};

// 测试结构体
struct WithValueType{
    using value_type = int;
};

struct WithoutValueType{};

struct WithVoidValueType{
    using value_type = void;
};

#endif  //SFINAE_DEMO_H
