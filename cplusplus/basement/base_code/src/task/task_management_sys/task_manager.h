
#ifndef TASK_TASK_MANAGER_H
#define TASK_TASK_MANAGER_H

#include "task.h"
#include <cstddef>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fstream>

class task_manager
{
    public:
    
        task_manager();
        
        void add(const std::string& description, int priority, const std::string& date);
        
        void remove(int id);
        
        void update(int id, const std::string& description, int priority, const std::string& date);
        
        void list(int option) const;
        
        void load();

        
        void save() const;
    
    private:
        
        std::vector<task> _tasks;
        size_t _next_id;

    private:

        static bool cmp_by_priority(const task& ta, const task& tb);
        
        static bool cmp_by_date(const task& ta, const task& tb);

};

#endif  // TASK_TASK_MANAGER_H
