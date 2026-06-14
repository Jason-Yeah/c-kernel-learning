
#ifndef TASK_COMMAND_H
#define TASK_COMMAND_H

#include <string>

struct command_base
{
    virtual void exec(const std::string& args) = 0;
};

template <typename Derived>
struct command: public command_base
{
    void exec(const std::string& args) override
    {
        (static_cast<Derived*>(this))->exec_impl(args);
    }
};

#include "task_manager.h"

class add_command: public command<add_command>
{
    public:
        
        add_command(task_manager& manager);

        void exec_impl(const std::string& args);

    private:

        task_manager& _t_manager;
};

class del_command: public command<del_command>
{
    public:
        del_command(task_manager& manager);
        
        void exec_impl(const std::string& args);
        
    private:
        task_manager& _t_manager;
};

class ls_command: public command<ls_command>
{
    public:

        ls_command(task_manager& manager);

        void exec_impl(const std::string& args);

    private:
        task_manager& _t_manager;
};

class update_command: public command<update_command>
{
    public:
        
        update_command(task_manager& manager);
        
        void exec_impl(const std::string& args);
        
    private:

        task_manager& _t_manager;
};

#endif  // TASK_COMMAND_H
