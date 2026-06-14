#include "task.h"

std::string task::_task_filename = "tasks.txt";

std::string task::to_string() const
{
    std::ostringstream oss;
    oss << "ID: " << _id << ", Description: "<< _description
        << ", Priority: " << _priority << ", Deadline: " << _ddl;
    return oss.str();
}