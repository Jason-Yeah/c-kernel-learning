#ifndef BASE_MEMORY_DARRY_DEMO_H
#define BASE_MEMORY_DARRY_DEMO_H

#include <iostream>

class DArray
{
    public:
        DArray();

        ~DArray();

        void add(int value);

        int get(size_t idx) const;

        size_t size() const;

        size_t capacity() const;

    private:
        size_t _capacity;
        size_t _size;
        int* _data;

        void resize(size_t new_capacity);
};

#endif