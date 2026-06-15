#include "command.h"
#include "task_manager.h"
#include "logger.h"
#include <cstddef>
#include <stdexcept>
#include <string>

add_command::add_command(task_manager& manager): _t_manager(manager) {}

void add_command::exec_impl(const std::string& args)
{
    size_t pos1 = args.find(',');
    size_t pos2 = args.find( ',', pos1 + 1);
    if (pos1 == std::string::npos || pos2 == std::string::npos)
    {
        std::cout << "Incorrect parameter format. " << 
            "Please use: add <description>, <priority>, <deadline>" << std::endl;
        return;
    }

    if (pos1 == 0)
    {
        std::cout << "Description cannot be empty." << std::endl;
        return;
    }

    std::string description = args.substr(0, pos1);
    std::string priority_str = args.substr(pos1 + 1, pos2 - pos1 - 1);
    std::string deadline = args.substr(pos2 + 1);

    int priority;
    try
    {
        priority = std::stoi(priority_str);
    }
    catch (std::invalid_argument&)
    {
        std::cout << "Priority must be a number (1/2/3)." << std::endl;
        return;
    }
    catch (std::out_of_range&)
    {
        std::cout << "Priority out of range." << std::endl;
        return;
    }

    _t_manager.add(description, priority, deadline);
    std::cout << "Successfully add task." << std::endl;
}

del_command::del_command(task_manager& manager): _t_manager(manager) {}

void del_command::exec_impl(const std::string& args)
{
    try
    {
        size_t idx;
        int id = std::stoi(args, &idx);
        if (idx != args.size())
        {
            std::cout << "Incorrect parameter format. Please use: delete <ID>\n";
            return ;
        }
        _t_manager.remove(id);
        std::cout << "Successfully delete task " << id << "." << std::endl;
    }
    catch (std::invalid_argument& iae)
    {
        std::cout << "Incorrect parameter format. Please use: delete <ID>\n";
        return;
    }
    catch (std::out_of_range& ofre)
    {
        std::cout << "ID was our of range. Please use valid task ID.";
        return;
    }
}


ls_command::ls_command(task_manager& manager): _t_manager(manager) {}

void ls_command::exec_impl(const std::string& args)
{
    int option = 0;
    if (!args.empty()) option = std::stoi(args);
    _t_manager.list(option);
}


update_command::update_command(task_manager& manager): _t_manager(manager) {}

void update_command::exec_impl(const std::string& args)
{
    // ID, d, p, ddl;
    size_t pos_description = args.find(',');
    size_t pos_priority = args.find(',', pos_description + 1);
    size_t pos_ddl = args.find(',', pos_priority + 1);

    if (pos_description == std::string::npos || pos_priority == std::string::npos
        || pos_ddl == std::string::npos)
    {
        std::cout << "Incorrect parameter format. Please use: update <ID>," 
        << " <DESCRIPTION>, <PRIORITY>, <DEADLINE>" << std::endl;
        return;
    }

    int id = std::stoi(args.substr(0, pos_description));
    std::string description = args.substr(pos_description + 1, pos_priority - pos_description - 1);
    int priority = std::stoi(args.substr(pos_priority + 1, pos_ddl - pos_priority - 1));
    std::string ddl = args.substr(pos_ddl + 1);
    
    _t_manager.update(id, description, priority, ddl);
    std::cout << "Successfully update task." << std::endl;
}