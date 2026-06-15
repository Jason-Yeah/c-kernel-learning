#include "logger.h"
#include "task_manager.h"
#include "command.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <variant>

int main()
{
    logger::instance().log("JY");
    task_manager tmgr;

    // PLAN_1: Virtual Based class
    // std::unordered_map<std::string, std::unique_ptr<command_base>> cmds;
    // cmds["add"] = std::make_unique<add_command>(tmgr);
    // cmds["delete"] = std::make_unique<del_command>(tmgr);
    // cmds["list"] = std::make_unique<ls_command>(tmgr);
    // cmds["update"] = std::make_unique<update_command>(tmgr);
    
    // PLAN_2: Type Erasure
    /**
    * Smart pointers captured by lambda expressions can effectively extend
    * the lifetime of local variables.
    * ---
    * `value::type` is std::function<T>. The actual data stored is of type lambda expression, 
    * and each lambda expression in C++ is a unique anonymous type. 
    * These four lambda expressions have completely different types, 
    * but `std::function` "erases" them all to the same type -> `std::function<void(const std::string&)>`
    */
    // std::unordered_map<std::string, std::function<void(const std::string&)>> cmds;
    // auto add_cmd = std::make_shared<add_command>(tmgr); 
    // auto ls_cmd = std::make_shared<ls_command>(tmgr); 
    // auto update_cmd = std::make_shared<update_command>(tmgr); 
    // auto delete_cmd = std::make_shared<del_command>(tmgr);
    // cmds["add"] = [add_cmd](const std::string& args){
    //     add_cmd->exec(args);
    // };
    // cmds["delete"] = [delete_cmd](const std::string& args){
    //     delete_cmd->exec(args);
    // };
    // cmds["list"] = [ls_cmd](const std::string& args){
    //     ls_cmd->exec(args);
    // };
    // cmds["update"] = [update_cmd](const std::string& args){
    //     update_cmd->exec(args);
    // };

    // PLAN_3: std::variant
    /**
    * Allows a variable to hold one of multiple predefined types at runtime.
    */
    // using cmd_variant = std::variant<
    //     std::unique_ptr<add_command>,
    //     std::unique_ptr<del_command>,
    //     std::unique_ptr<ls_command>,
    //     std::unique_ptr<update_command>
    // >;
    // std::unordered_map<std::string, cmd_variant> cmds;
    // cmds["add"] = std::make_unique<add_command>(tmgr);
    // cmds["delete"] = std::make_unique<del_command>(tmgr);
    // cmds["list"] = std::make_unique<ls_command>(tmgr);
    // cmds["update"] = std::make_unique<update_command>(tmgr);

    //PLAN_4 Wapper
    std::unordered_map<std::string, wapper_command> cmds;
    cmds.emplace("add", add_command(tmgr));
    cmds.emplace("list", ls_command(tmgr));
    cmds.emplace("delete", del_command(tmgr));
    cmds.emplace("update", update_command(tmgr));

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
        if (it != cmds.end()) 
        {
            it->second.exec(args);
            // 在泛型lambda中，auto&&是转发引用，适配左右值
            // std::visit([&args](auto&& cmd_p){
            //     cmd_p->exec(args);
            // }, it->second);
        }
        else std::cout << "Unknown command: " << cmd << std::endl;
    }

    return 0;
}