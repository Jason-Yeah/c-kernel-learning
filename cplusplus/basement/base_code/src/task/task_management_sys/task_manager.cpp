#include "task_manager.h"
#include "logger.h"
#include "task.h"
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

task_manager::task_manager(): _next_id(1)
{
    load();
}

void task_manager::add(const std::string& description, int priority, const std::string& date)
{
    task tsk;
    tsk._id = _next_id ++ ;
    tsk._priority = priority;
    tsk._description = description;
    tsk._ddl = date;
    logger::instance().log("Task added: " + tsk.to_string()); 
    _tasks.push_back(tsk);
    save();
}

void task_manager::remove(int id)
{
    // 泛型算法，当谓词返回true，find_if会返回其对应的迭代器否则是end()
    auto it = std::find_if(_tasks.begin(), _tasks.end(), [id](const task& tsk){
        return tsk._id == id;
    });

    if (it == _tasks.end()) // 做return处理防止下面erase报错出现Undefined behavior
    {
        std::cout << "Task not found." << std::endl;
        logger::instance().log("Task not found.");
        return; // 优化
    }
    
    logger::instance().log("Task deleted: " + it->to_string());
    _tasks.erase(it);
    save();
}

void task_manager::update(int id, const std::string& description, int priority, const std::string& date)
{
    auto it = std::find_if(_tasks.begin(), _tasks.end(), [id](const task& tsk){
        return tsk._id == id;
    });

    if (it == _tasks.end())
    {
        std::cout << "Task not found." << std::endl;
        logger::instance().log("Task not found.");
        return; // 优化
    }
    it->_description = description;
    it->_priority = priority;
    it->_ddl = date;
    logger::instance().log("Task updated: " + it->to_string());
    save();
}

void task_manager::list(int option) const
{
    auto sort_list = _tasks;
    switch (option)
    {
        case 1: 
            std::sort(sort_list.begin(), sort_list.end(), cmp_by_priority);
            break;
        case 2:
            std::sort(sort_list.begin(), sort_list.end(), cmp_by_date);
            break;
        default:
            break;
    }
    for (const auto& t: sort_list)
        std::cout << t.to_string() << std::endl;       
}

void task_manager::save() const
{
    std::ofstream ofs(task::_task_filename);
    if (!ofs.is_open())
    {
        logger::instance().log("Failed to open tasks file for writing.");
        return;
    }
    for (const auto& item: _tasks)
        ofs << item._id << ", " << item._description << ", " << item._priority << ", " << item._ddl << std::endl;

    ofs.close();
    logger::instance().log("Tasks saved successfully.");
}

void task_manager::load()
{
    std::ifstream ifs(task::_task_filename);
    if (!ifs.is_open())
    {
        logger::instance().log("Falied to open tasks file.");
        return;
    }

    std::string line;
    while (std::getline(ifs, line))
    {
        std::istringstream iss(line);
        task tsk;
        char delimiter;

        // iss中读取指针会自动向前移动
        iss >> tsk._id >> delimiter;
        std::getline(iss, tsk._description, ',');
        {
            size_t s = tsk._description.find_first_not_of(" \t");
            size_t e = tsk._description.find_last_not_of(" \t");
            tsk._description = (s == std::string::npos) ? "" :
            tsk._description.substr(s, e - s + 1);
        }
        iss >> tsk._priority >> delimiter;
        iss >> tsk._ddl;

        _tasks.push_back(tsk);
        if (tsk._id >= _next_id) _next_id = tsk._id + 1;
    }

    ifs.close();
    logger::instance().log("Tasks loaded successfully.");
}

bool task_manager::cmp_by_priority(const task& ta, const task& tb)
{
    return ta._priority < tb._priority;
}

bool task_manager::cmp_by_date(const task& ta, const task& tb)
{
    return ta._ddl < tb._ddl;
}

void task_manager::filter(const filter_args& args) const
{
    std::vector<std::function<bool(const task&)>> preds;

    if (!args._keyword.empty())
        preds.push_back([&args](const task& t){
            return t._description.find(args._keyword) != std::string::npos;
    });

    if (args.priority != 0)
        preds.push_back([&args](const task& t){
            return t._priority == args.priority;
    });

    if (!args._data_st.empty() && !args._data_ed.empty())
        preds.push_back([&args](const task& t){
            return t._ddl >= args._data_st && t._ddl <=args._data_ed;
    });

    if (preds.empty())
    {
            // 没有任何过滤条件，提示用法
            std::cout << "Usage: filter -k <keyword> -p <priority>"
            << "-d <date_begin> <date_end>" << std::endl;
            return;
    }
    
    for (const auto& t: _tasks)
    {
        // std::all_of检查所有元素是否满足指定条件（谓词）
        bool is_match = std::all_of(preds.begin(), preds.end(),
         [&t](const auto& pred){
            return pred(t);
        });

        if (is_match)
            std::cout << t.to_string() << std::endl;
    }
}
