#include "logger.h"
#include "task_manager.h"
#include "command.h"
#include <string>
#include <unordered_map>
#include <memory>

int main()
{
    logger::instance().log("JY");
    task_manager tmgr;

    std::unordered_map<std::string, std::unique_ptr<command_base>> cmds;
    
    cmds["add"] = std::make_unique<add_command>(tmgr);
    cmds["delete"] = std::make_unique<del_command>(tmgr);
    cmds["list"] = std::make_unique<ls_command>(tmgr);
    cmds["update"] = std::make_unique<update_command>(tmgr);
    
    std::cout << "Welcome! Please input commands: " << std::endl;
    std::string input;

    while(true)
    {
        std::getline(std::cin, input);
        if (input.empty()) continue;

        size_t space_pos = input.find(' ');
        std::string cmd = input.substr(0, space_pos);
        std::string args;
        if (space_pos != std::string::npos)
            args = input.substr(space_pos + 1); // 去前空白

        if (cmd == "exit")
        {
            std::cout << "Exiting program..." << std::endl;
            break;
        }

        auto it = cmds.find(cmd);
        if (it != cmds.end()) it->second->exec(args);
        else std::cout << "Unknown command: " << cmd << std::endl;
    }

    return 0;
}