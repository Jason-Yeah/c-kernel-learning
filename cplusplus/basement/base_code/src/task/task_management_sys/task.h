
#ifndef TASK_TASK_H
#define TASK_TASK_H

#include <string>
#include <sstream>

struct task
{
    int _id;
    std::string _description;
    int _priority; // 1 high 2 mid 3 low
    std::string _ddl; // YYYY-MM-DD
    static std::string _task_filename;

    std::string to_string() const;
};

#endif  // TASK_TASK_H
