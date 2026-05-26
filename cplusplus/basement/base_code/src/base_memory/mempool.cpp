#include "mempool.h"

MemPool::MemPool(size_t obj_size, size_t total_size)
: _obj_size(obj_size), _total_size(total_size)
{
    _pool = (char*)malloc(obj_size * total_size);
    if (_pool == nullptr) throw std::bad_alloc{};

    // Init free_stk
    for (size_t i = 0; i < total_size; i ++ )
        _free_stk.push(_pool + i * _obj_size);
}

MemPool::~MemPool()
{
    free(_pool);
}

void* MemPool::allocate()
{
    if (_free_stk.empty()) return nullptr;
    auto p = _free_stk.top();
    _free_stk.pop();
    return p;
}

void MemPool::deallocate(void* ptr)
{
    _free_stk.push(ptr);
}

int main()
{
    MemPool pool(sizeof(Stu), 3);
    void* mem1 = pool.allocate();
    void* mem2 = pool.allocate();

    auto obj1 = new(mem1) Stu("Jason", 23);
    auto obj2 = new(mem2) Stu("Tom", 53);

    std::cout << obj1->name_ << ' ' << obj1->age_ << std::endl;
    std::cout << obj2->name_ << ' ' << obj2->age_ << std::endl;

    obj1->~Stu();
    obj2->~Stu();

    pool.deallocate(mem1);
    pool.deallocate(mem2);

    return 0;
}
