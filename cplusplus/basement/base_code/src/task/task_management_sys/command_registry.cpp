#include "command_registry.h"

command_registry& command_registry::instance()
{
    static command_registry _inst;
    return _inst;
}

void command_registry::register_command(const std::string& name, creator_t creator)
{
    _creators[name] = std::move(creator);
}

wapper_command command_registry::create(const std::string& name, task_manager& tmgr)
{
    auto it = _creators.find(name);
    if (it != _creators.end())
        return it->second(tmgr);

    throw std::runtime_error("Unknown command: " + name);
}
