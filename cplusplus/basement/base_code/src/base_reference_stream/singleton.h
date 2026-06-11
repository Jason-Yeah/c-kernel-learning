
#ifndef SINGLETON_H
#define SINGLETON_H

#include <iostream>
#include <mutex>
#include <memory>
#include <thread>

/**
* C++ 98
*/
namespace sen_c98 
{
    
    /**
    * General style
    */
    class singleton
    {
        public:
            ~singleton()
            {
                std::cout << "singleton(c++98/11) destructed" << std::endl;
            }

            /*
            * C++11 guarantees thread safety during the initialization of 
            * static local variables.
            */
            static singleton& instance()
            {
                static singleton inst;
                return inst;
            }

        private:
            singleton(){}
            // singleton(const singleton&){}
            singleton(const singleton&) = delete;
            singleton& operator=(const singleton&) = delete;
            singleton(singleton&&) = delete;
            singleton& operator=(const singleton&&) = delete;
    };

    /**
    * Hungry man style
    */
    class singleton_hungry
    {
        private:
            singleton_hungry(){}
            singleton_hungry(const singleton_hungry&) = delete;
            singleton_hungry(singleton_hungry&&) = delete;
            singleton_hungry& operator= (const singleton_hungry&) = delete;
            singleton_hungry& operator= (singleton_hungry&&) = delete;

            static singleton_hungry* inst;
        public:
            ~singleton_hungry()
            {
                std::cout << "singleton_hungry destructed.\n";
            }

            static singleton_hungry* instance()
            {
                if (inst == nullptr) inst = new singleton_hungry();
                return inst;
            }
    };

    /**
    * Lazy man style
    */
    class singleton_lazy
    {
        public:
            ~singleton_lazy()
            {
                std::cout << "singleton_lazy destructed.\n";
            }

            static singleton_lazy* instance()
            {
                if (inst != nullptr) return inst;

                _mtx.lock();
                if (inst != nullptr)
                {
                    _mtx.unlock();
                    return inst;
                }

                inst = new singleton_lazy();

                _mtx.unlock();
                return inst;
            }

        private:
            singleton_lazy(){}
            singleton_lazy(const singleton_lazy&) = delete;
            singleton_lazy& operator= (const singleton_lazy&) = delete;
            singleton_lazy(singleton_lazy&&) = delete;
            singleton_lazy& operator= (singleton_lazy&&) = delete;

            static singleton_lazy* inst;
            static std::mutex _mtx;
    };
};

/**
* C++ 11
*/
namespace sen_c11 
{
    class singleton
    {
        public:
            ~singleton()
            {
                std::cout << "Singleton ONCE_FLAG destructed.\n";
            }

            static std::shared_ptr<singleton> instance()
            {
                static std::once_flag _flag;
                std::call_once(_flag, [](){
                    // _inst = new singleton();
                    _inst = std::shared_ptr<singleton>(new singleton());
                });
                return _inst;
            }

        private:
            singleton() = default;
            singleton(const singleton&) = delete;
            singleton(singleton&&) = delete;
            singleton& operator= (const singleton&) = delete;
            singleton& operator= (singleton&&) = delete;

            // static singleton* _inst;
            static std::shared_ptr<singleton> _inst;
    };
};

namespace CRTP
{
    template <typename T>
    class singleton
    {
        protected:
            singleton() = default;
            singleton(const singleton&) = delete;
            singleton(singleton&&) = delete;
            singleton& operator= (const singleton&) = delete;
            singleton& operator= (singleton&&) = delete;

            static std::shared_ptr<T> _inst;
        public:
            ~singleton() = default;
            static std::shared_ptr<T> instance()
            {
                static std::once_flag _flag;
                std::call_once(_flag, [](){
                    _inst = std::shared_ptr<T>(new T());
                });
                return _inst;
            }
    };

    /**
    * Static member variable definitions in template classes
    * must be placed in an inline file(.h/hpp).
    */
    template <typename T>
    std::shared_ptr<T> singleton<T>::_inst = nullptr;

    class single_net: public singleton<single_net>
    {
        friend class singleton<single_net>;

        private:
            single_net() = default;
        public:
            ~single_net()
            {
                puts("Single net destruted.");
            }
    };
};

#endif  // SINGLETON_H
