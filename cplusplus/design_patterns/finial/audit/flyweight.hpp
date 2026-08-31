#pragma once

#include <map>
#include <memory>
#include <string>

class FileType // .cpp .hpp ...
{
public:
    // .cpp source
    FileType(std::string extension, std::string category)
        : extension_(std::move(extension)), category_(std::move(category))
    {
    }

    const std::string &extension() const { return extension_; }
    const std::string &category() const { return category_; }

private:
    std::string extension_; // 内部状态：可共享且不可变
    std::string category_;
};

class FileTypeFactory
{
    std::map<std::string, std::shared_ptr<const FileType>> cache_;

public:
    std::shared_ptr<const FileType> get(const std::string &extension,
                                        const std::string &category)
    {
        auto it = cache_.find(extension);
        if (it != cache_.end())
            return it->second;
        auto type = std::make_shared<const FileType>(extension, category);
        cache_[extension] = type;
        return type;
    }

    std::size_t typeCount() const { return cache_.size(); }
};
