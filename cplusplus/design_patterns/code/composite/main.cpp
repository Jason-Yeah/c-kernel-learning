#include "composite.hpp"
#include "leaf.hpp"

int main()
{
    using fsnode_t = std::unique_ptr<FileSysNode>;

    auto root = std::make_unique<Directory>("root"); // Composite

    root->Add(std::make_unique<File>("readme.md", 100));

    auto src = std::make_unique<Directory>("src");      // Composite
    src->Add(std::make_unique<File>("main.cpp", 2000)); // Leaf
    src->Add(std::make_unique<File>("utils.h", 500));   // Leaf

    auto images = std::make_unique<Directory>("images");   // Composite
    images->Add(std::make_unique<File>("logo.png", 8000)); // Leaf
    images->Add(std::make_unique<File>("bg.jpg", 15000));  // Leaf

    src->Add(std::move(images)); // Composite 套 Composite（目录套目录）
    root->Add(std::move(src));

    // ★ 客户端只调用 Component 接口（GetSize），从不判断对象是 Leaf 还是
    // Composite！
    std::cout << "=== 计算 root 目录总大小 ===" << std::endl;
    size_t total = root->GetSize(); // 对 Composite 调用 → 递归展开
    std::cout << "\n总大小: " << total << "B" << std::endl;

    return 0;
}