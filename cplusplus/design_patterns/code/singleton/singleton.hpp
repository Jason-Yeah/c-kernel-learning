#pragma once

#include <iostream>

class Singleton98
{
public:
    static Singleton98 *GetInstance()
    {
        // 多线程问题，俩都new了
        if (inst_ == NULL)
            inst_ = new Singleton98();
        // new Singleton() 永远没人 delete
        return inst_;
    }

    void DoSomething() const
    {
        std::cout << "  [方法] 单例在做事情（地址: " << this << "）"
                  << std::endl;
    }

private:
    static Singleton98 *inst_;

    Singleton98() { std::cout << "  [构造] 实例已创建" << std::endl; }
    ~Singleton98() { std::cout << "  [析构] 实例已销毁" << std::endl; }
    Singleton98(const Singleton98 &);            // 拷贝构造
    Singleton98 &operator=(const Singleton98 &); // 赋值运算符
};

class Singleton11
{
public:
    // 全局访问点 —— ★ 函数内 static 局部变量 ★
    static Singleton11 &GetInstance()
    {
        // C++11 起：函数内静态局部变量的初始化是线程安全的！
        // 首次调用时创建，之后直接返回；程序结束时自动析构
        static Singleton11 instance;
        return instance;
    }

    void DoSomething() const
    {
        std::cout << "  [方法] 单例做事（地址: " << this << "）" << std::endl;
    }

private:
    // 私有构造函数
    // 禁止拷贝（C++11 新语法）
    Singleton11(const Singleton11 &) = delete;
    Singleton11 &operator=(const Singleton11 &) = delete;

    Singleton11() { std::cout << "  [构造] 实例已创建" << std::endl; }
    ~Singleton11() { std::cout << "  [析构] 实例已销毁" << std::endl; }
};

template <typename T> class SingletonBase
{
protected:
    SingletonBase() = default;
    ~SingletonBase() = default;

public:
    SingletonBase(const SingletonBase &) = delete;
    SingletonBase &operator=(const SingletonBase &) = delete;

    static T &GetInstance()
    {
        static T instance;
        return instance;
    }
};

#include <map>

class ConfigManager : public SingletonBase<ConfigManager>
{
public:
    void Set(const std::string &k, const std::string &v) { map_[k] = v; }
    std::string Get(const std::string &k) const { return map_.at(k); }

private:
    std::map<std::string, std::string> map_;
    // 注意：构造函数要私有，防止直接实例化
    ConfigManager() = default;
    friend class SingletonBase<ConfigManager>; // 基类需要访问私有构造
};

class Logger : public SingletonBase<Logger>
{
public:
    void Log(const std::string &msg)
    {
        std::cout << "[LOG] " << msg << std::endl;
    }

private:
    Logger() = default;
    friend class SingletonBase<Logger>;
};