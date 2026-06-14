#include "task_manager.h"
#include "logger.h"
#include "task.h"
#include <algorithm>
#include <fstream>
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

    if (it == _tasks.end())
    {
        std::cout << "Task not found." << std::endl;
        logger::instance().log("Task not found.");
    }
    _tasks.erase(it);
    logger::instance().log("Task deleted: " + it->to_string());
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