#ifndef BASE_MEM_SIMPLE_SHARED_PTR_H
#define BASE_MEM_SIMPLE_SHARED_PTR_H

#include <iostream>
#include <string>
#include <atomic>

class Stu
{
    public:
        Stu() {}
        Stu(const std::string name, int age): name_(name), age_(age){}
        ~Stu()
        {
            std::cout << "Stu " << name_ << "~" << std::endl;
        }
        std::string name_;
        int age_;
};

struct ControlBlock
{
    std::atomic<int> _ref_cnt;
    ControlBlock() : _ref_cnt(1) {}
};

template <typename T>
class SimpleSharedPtr
{
    private:
        T* _ptr;
        ControlBlock* _control;
        void release();
    public:
        SimpleSharedPtr();
    /*
        没有 explicit：编译器会把单参数构造函数当成“自动类型转换器”，在内存管理中极易导致所有权混乱、双重释放、悬垂指针。
        有 explicit：强制程序员把意图写在明面上，杜绝编译器在背后偷偷创建临时智能指针对象。
    */
    // explicit显示转换，只能修饰构造函数和类型转换运算符。
    // 不可以SimpleSharedPtr p = new xxx(); 而是Simple.. p(new XXX());
        explicit SimpleSharedPtr(T* ptr);

        ~SimpleSharedPtr();

        SimpleSharedPtr(const SimpleSharedPtr& other);

        SimpleSharedPtr& operator = (const SimpleSharedPtr& other);

        SimpleSharedPtr(SimpleSharedPtr&& other) noexcept;

        SimpleSharedPtr& operator = (SimpleSharedPtr&& other);

        T* operator -> () const noexcept;

        T& operator *() const noexcept;

        // Clazz *raw_cp = sp.get();
        T* get() const;

        int use_cnt() const;

        void reset(T* );
};

template <typename T>
void SimpleSharedPtr<T>::release()
{
    if (_control)
    {
        _control->_ref_cnt -- ;
        if (_control->_ref_cnt == 0)
        {
            delete _ptr;
            _ptr = nullptr;
            delete _control;
            _control = nullptr;
        }
    }
}

template <typename T>
SimpleSharedPtr<T>::SimpleSharedPtr() : _ptr(nullptr), _control(nullptr) {}

template <typename T>
SimpleSharedPtr<T>::SimpleSharedPtr(T* ptr) : _ptr(ptr)
{
    if (ptr) _control = new ControlBlock();
    else _control = nullptr;
}

template <typename T>
SimpleSharedPtr<T>::~SimpleSharedPtr()
{
    this->release();
}

template <typename T>
SimpleSharedPtr<T>::SimpleSharedPtr(const SimpleSharedPtr<T>& other) 
: _ptr(other._ptr), _control(other._control)
{
    if (_control) _control->_ref_cnt ++ ;
}

template <typename T>
SimpleSharedPtr<T>& SimpleSharedPtr<T>::operator = (const SimpleSharedPtr<T>& other)
{
    if (this != &other)
    {
        this->release();
        _ptr = other._ptr;
        _control = other._control;
        if (_control) _control->_ref_cnt ++ ;
    }
    return *this;
}

template <typename T>
SimpleSharedPtr<T>::SimpleSharedPtr(SimpleSharedPtr<T>&& other) noexcept
: _ptr(other._ptr), _control(other._control)
{
    other._ptr = nullptr;
    other._control = nullptr;
}

template<typename T>
SimpleSharedPtr<T>& SimpleSharedPtr<T>::operator= (SimpleSharedPtr<T>&& other)
{
    if (this != &other)
    {
        this->release();
        _ptr = other._ptr;
        _control = other._control;
        other._ptr = nullptr;
        other._control = nullptr;
    }
    return *this;
}

template <typename T>
T* SimpleSharedPtr<T>::operator -> () const noexcept
{
    return _ptr;
}

template <typename T>
T& SimpleSharedPtr<T>:: operator * () const noexcept
{
    return *_ptr;
}

template <typename T>
T* SimpleSharedPtr<T>::get() const
{
    return _ptr;
}

template <typename T>
int SimpleSharedPtr<T>::use_cnt() const
{
    return (_control) ? _control->_ref_cnt.load() : 0;
}

template <typename T>
void SimpleSharedPtr<T>:: reset(T* objp)
{
    if (_ptr == objp) return;

    release();
    _ptr = objp;
    if (objp)
        _control = new ControlBlock();
    else
        _control = nullptr;
}

#endif
