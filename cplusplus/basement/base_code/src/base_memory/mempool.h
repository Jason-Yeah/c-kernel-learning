#ifndef BASE_MEM_MEMPOOL_H
#define BASE_MEM_MEMPOOL_H

#include <iostream>
#include <stack>

class MemPool 
{
    public:
        MemPool(size_t , size_t );
        ~MemPool();
        void* allocate();
        void deallocate(void* );
    private:
        size_t _obj_size;
        size_t _total_size;
        char* _pool;
        // Using stack to manage elements in the memory pool.
        std::stack<void*> _free_stk;
};

// Just for test
class Stu
{
    public:
        Stu(const std::string name, int age): name_(name), age_(age){}
        std::string name_;
        int age_;
};

#endif