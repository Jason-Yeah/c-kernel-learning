#pragma once

#include "button.hpp"
#include "dialog.hpp"
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class GUIFactory
{
public:
    virtual ~GUIFactory() = default;
    virtual std::unique_ptr<Button> create_button() = 0;
    virtual std::unique_ptr<Dialog> create_dialog() = 0;
};

class WinFactory : public GUIFactory
{
public:
    std::unique_ptr<Button> create_button() override
    {
        return std::make_unique<WinButton>();
    }
    std::unique_ptr<Dialog> create_dialog() override
    {
        return std::make_unique<WinDialog>();
    };
};

class LinuxFactory : public GUIFactory
{
public:
    std::unique_ptr<Button> create_button() override
    {
        return std::make_unique<LinuxButton>();
    }
    std::unique_ptr<Dialog> create_dialog() override
    {
        return std::make_unique<LinuxDialog>();
    }
};

class GUIFactoryCreator
{
public:
    static std::unique_ptr<GUIFactory> create(const std::string &os)
    {
        if (os == "Windows")
            return std::make_unique<WinFactory>();
        if (os == "Linux")
            return std::make_unique<LinuxFactory>();
        throw std::invalid_argument("Unknown OS: " + os);
    }
};

class FactoryRegister
{
public:
    using creator_func = std::function<std::unique_ptr<GUIFactory>()>;

    static void reg(const std::string &key, creator_func creator)
    {
        instance().emplace(key, std::move(creator));
    }

    static std::unique_ptr<GUIFactory> create(const std::string &key)
    {
        auto &r = instance();
        auto it = r.find(key);
        if (it == r.end())
            throw std::runtime_error("Unknown factory: " + key);

        return it->second();
    }

private:
    static std::unordered_map<std::string, creator_func> &instance()
    {
        static std::unordered_map<std::string, creator_func> inst;
        return inst;
    }
};

void init_factory_register()
{
    FactoryRegister::reg("win", [] { return std::make_unique<WinFactory>(); });

    FactoryRegister::reg("linux",
                         [] { return std::make_unique<LinuxFactory>(); });
}

struct AutoRegister
{
    AutoRegister(const std::string &key, FactoryRegister::creator_func creator)
    {
        FactoryRegister::reg(key, std::move(creator));

        std::cout << "  [自注册] 工厂 \"" << key << "\" 已就绪" << std::endl;
    }
};

static AutoRegister g_winreg("win",
                             [] { return std::make_unique<WinFactory>(); });
