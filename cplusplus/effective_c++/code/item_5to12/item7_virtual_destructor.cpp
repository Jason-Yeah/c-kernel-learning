#include <iostream>
#include <memory>

class Plugin
{
public:
    virtual void run() const = 0;
    virtual ~Plugin() { std::cout << "Plugin destructor\n"; }
};

class FilePlugin final : public Plugin
{
public:
    void run() const override { std::cout << "FilePlugin runs\n"; }
    ~FilePlugin() override { std::cout << "FilePlugin destructor\n"; }
};

int main()
{
    std::unique_ptr<Plugin> plugin = std::make_unique<FilePlugin>();
    plugin->run();
} // unique_ptr 删除 Plugin*：先调用 FilePlugin 析构，再调用 Plugin 析构
