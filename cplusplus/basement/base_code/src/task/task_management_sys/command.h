
#ifndef TASK_COMMAND_H
#define TASK_COMMAND_H

#include <algorithm>
#include <memory>
#include <string>

// struct command_base
// {
//     virtual void exec(const std::string& args) = 0;
// };

class wapper_command
{
    public:

        template<typename T>
        wapper_command(T cmd): impl(std::make_unique<model<T>>(std::move(cmd))){}
        void exec(const std::string& args)
        {
            impl->exec(args);
        }

    private:

        struct _concept
        {
            virtual void exec(const std::string& args) = 0;
            virtual ~_concept() = default;
        };

        template<typename T>
        struct model: public _concept
        {
            T _cmd;
            model(T cmd): _cmd(std::move(cmd)) {}
            void exec(const std::string& args) override
            {
                _cmd.exec(args);
            }
        };
    private:
        std::unique_ptr<_concept> impl;
};

template <typename Derived>
struct command
{
    void exec(const std::string& args)
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
