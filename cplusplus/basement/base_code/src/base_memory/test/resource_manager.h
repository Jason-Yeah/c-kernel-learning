#ifndef _TEST_RESOURCE_MANAGER_H
#define _TEST_RESOURCE_MANAGER_H

#include <iostream>
#include <memory>

/**
 * 题目描述：
编写一个C++类 ResourceManager，该类负责管理一个动态分配的整数资源。使用 std::unique_ptr 来确保资源在 ResourceManager 对象销毁时被正确释放，从而避免内存泄漏。
要求：
1. 类定义：
  ○ 创建一个名为 ResourceManager 的类。
  ○ 添加一个私有成员变量，类型为 std::unique_ptr<int>，用于管理动态分配的整数。
2. 构造函数与析构函数：
  ○ 实现构造函数，接受一个整数参数并初始化 std::unique_ptr<int>。
  ○ 不需要显式定义析构函数，依赖 std::unique_ptr 自动释放资源。
3. 成员函数：
  ○ int getValue() const;
返回当前管理的整数值。
  ○ void setValue(int newValue);
设置整数的新值。
4. 禁止拷贝：
  ○ 禁用拷贝构造函数和拷贝赋值运算符，确保 ResourceManager 对象不能被拷贝。
  ○ 允许移动构造和移动赋值运算符。
5. 测试：
  ○ 编写一个 main 函数，创建 ResourceManager 对象，设置并获取值，展示智能指针的作用。
 */
class resource_manager
{
    private:
        std::unique_ptr<int> _dintp;
    public:
        resource_manager();
        resource_manager(int val);
        resource_manager(const resource_manager& other) = delete;
        resource_manager& operator= (const resource_manager& other) = delete;
        resource_manager(resource_manager&& other) noexcept;
        resource_manager& operator= (resource_manager&& other) noexcept;

        int get_value() const;
        void set_value(int new_val);
};

#endif  // _TEST_RESOURCE_MANAGER_H
