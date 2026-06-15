
#ifndef TASK_COMMAND_REGISTRY_H
#define TASK_COMMAND_REGISTRY_H

#include "command.h"
#include "task_manager.h"
#include <string>
#include <functional>
#include <unordered_map>

class command_registry
{
    public:
        
        using creator_t = std::function<wapper_command(task_manager&)>;

        static command_registry& instance();

        void register_command(const std::string& name, creator_t creator);

        wapper_command create(const std::string& name, task_manager& tmgr);

    private:
        
        std::unordered_map<std::string, creator_t> _creators;
};

#endif  // TASK_COMMAND_REGISTRY_H
