
#ifndef REFERENCE_DEMO_H
#define REFERENCE_DEMO_H

#include <ios>
#include <iostream>
#include <type_traits>
#include <utility>

void func(int& x)
{
    std::cout << "func(int&)" << std::endl;
}

void func(int&& x)
{
    std::cout << "func(int&&)" << std::endl;
}

template <typename T>
void wrapper(T&& arg)
{
    func(std::forward<T>(arg));
}

template <typename T>
void check_ref(T&& x)
{
    std::cout << std::boolalpha;
    std::cout << "T is lvalue reference: " << std::is_lvalue_reference<T>::value << std::endl;
    std::cout << "T is rvalue reference: " << std::is_rvalue_reference<T>::value << std::endl;
    std::cout << "T is rvalue : " << std::is_rvalue_reference<T&&> ::value << std::endl;
    std::cout << "x is lvalue reference: " << std::is_lvalue_reference<decltype(x)>::value << std::endl;
    std::cout << "x is rvalue reference: " << std::is_rvalue_reference<decltype(x)>::value << std::endl;
}

template <typename T>
void deduce_type(T&&)
{
    std::cout << std::boolalpha;
    std::cout << "Is T an lval ref: " << std::is_lvalue_reference_v<T> << std::endl;
    std::cout << "Is T an rval ref: " << std::is_rvalue_reference_v<T> << std::endl;
}

template <typename T>
void display_type(T&& x)
{
    std::cout << "T is lval ref: " << std::is_lvalue_reference_v<T> << " "
    << "x is lval ref: " << std::is_lvalue_reference_v<decltype(x)> << " "
    << "x is int: " << std::is_integral_v<typename std::remove_reference_t<T>> << std::endl;
}

template <typename F, typename T1, typename T2>
void flip1(F f, T1 t1, T2 t2)
{
    f(t2, t1);
}

template <typename F, typename T1, typename T2>
void flip2(F f, T1&& t1, T2&& t2)
{
    f(t2, t1);
}

void ftmp(int v1, int& v2)
{
    std::cout << v1 << " " << ++ v2 << std::endl;
}

void use_ftmp()
{
    int i = 100;
    int j = 99;
    flip1(ftmp, j, 42);
    std::cout << "j: " << j << std::endl;
    std::cout << "i: " << i << std::endl;
}

void use_ftmp2()
{
    int i = 100;
    int j = 99;
    flip2(ftmp, j, 42);
    std::cout << "j: " << j << std::endl;
    std::cout << "i: " << i << std::endl;
}

#endif  // REFERENCE_DEMO_H
 