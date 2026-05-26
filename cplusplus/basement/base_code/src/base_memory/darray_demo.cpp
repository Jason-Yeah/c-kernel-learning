#include "darray_demo.h"


DArray::DArray(): _capacity(2), _size(0), _data((int*)malloc(_capacity * sizeof(int)))
{
    // Check the alloced space of _data*
    if (_data == NULL)    
    {
        std::cerr << "malloc fail." << std::endl;
        // Constructor is failed. We shouldn't let it return -> throw an exception.
        throw std::bad_alloc{};
    }
}

DArray::~DArray()
{
    free(_data);
}

void DArray::add(int val)
{
    if (_size == _capacity)
    {
        resize(_capacity * 2);
    }

    _data[_size ++ ] = val;
}

int DArray::get(size_t idx) const
{
    if (idx >= _size) throw std::out_of_range{"Index out of range"};
    return _data[idx];
}

size_t DArray::size() const
{
    return _size;
}

void DArray::resize(size_t new_capacity)
{
    int* temp = (int*)realloc(_data, new_capacity * sizeof(int));
    if (temp == NULL) throw std::bad_alloc{};
    _data = temp;
    _capacity = new_capacity;
}


size_t DArray::capacity() const
{
    return _capacity;
}


int main()
{
    try
    {
        DArray* dap = new DArray();

        std::cout << dap->capacity() << std::endl;
        dap->add(10);
        std::cout << dap->capacity() << std::endl;
        dap->add(20);
        std::cout << dap->capacity() << std::endl;
        dap->add(30);
        std::cout << dap->capacity() << std::endl;

        for (size_t i = 0; i < dap->size(); i ++ )
            std::cout << dap->get(i) << std::endl;

        delete dap;
    }
    catch(const std::bad_alloc& e)
    {
        std::cerr << "Memory allocatoin error: " << e.what() << '\n';
        return -1;
    }
    catch(const std::out_of_range& e)
    {
        std::cerr << "Out of range error: " << e.what() << '\n';
        return -1;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Other error: " << e.what() << '\n';
        return -1;
    }
    
    int *ip = new int;
    
    char buf[10];
    char *cp = buf;

    std::cout << (void *)cp << ' ' << (void *)(cp + 1) << std::endl;
    std::cout << ip << ' ' << ip + 1 << std::endl;
    std::cout << sizeof(char *) << std::endl;

    return 0;
}
