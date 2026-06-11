#include "singleton.h"

// 静态成员定义（分配存储空间）
sen_c98::singleton_hungry* sen_c98::singleton_hungry::inst = nullptr;

auto inst = sen_c98::singleton_hungry::instance();

// init
// sen_c11::singleton* sen_c11::singleton::_inst = sen_c11::singleton::instance();
std::shared_ptr<sen_c11::singleton> sen_c11::singleton::_inst = nullptr;
// std::shared_ptr<sen_c11::singleton> sen_c11::singleton::_inst = sen_c11::singleton::instance();

int main()
{
    // std::cout << "s1 addr is " << &sen_c98::singleton::instance() << std::endl;
    // std::cout << "s2 addr is " << &sen_c98::singleton::instance() << std::endl;

    std::mutex mutex;
    std::thread t1([&mutex](){
        sen_c98::singleton::instance();
        std::lock_guard<std::mutex> lock(mutex);
        // mutex.lock();
        std::cout << "s1 addr is " << &sen_c98::singleton::instance() << std::endl;
        // mutex.unlock();
    });

    std::thread t2([&mutex](){
        sen_c98::singleton::instance();
        std::lock_guard<std::mutex> lock(mutex);
        // mutex.lock();
        std::cout << "s2 addr is " << &sen_c98::singleton::instance() << std::endl;
        // mutex.unlock();
    });


    std::thread t3([&mutex](){
        sen_c98::singleton_hungry::instance();
        std::lock_guard<std::mutex> lock(mutex);
        // mutex.lock();
        std::cout << "s3 addr is " << sen_c98::singleton_hungry::instance() << std::endl;
        // mutex.unlock();
    });

    std::thread t4([&mutex](){
        sen_c98::singleton_hungry::instance();
        std::lock_guard<std::mutex> lock(mutex);
        // mutex.lock();
        std::cout << "s4 addr is " << sen_c98::singleton_hungry::instance() << std::endl;
        // mutex.unlock();
    });

    std::thread t5([&mutex](){
        sen_c11::singleton::instance();
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "s5 addr is " << sen_c11::singleton::instance() << std::endl;
    });

    std::thread t6([&mutex](){
        sen_c11::singleton::instance();
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "s6 addr is " << sen_c11::singleton::instance() << std::endl;
    });

    std::thread t7([&mutex](){
        CRTP::single_net::instance();
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "s7 addr is " << CRTP::single_net::instance() << std::endl;
    });

    std::thread t8([&mutex](){
        CRTP::single_net::instance();
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "s8 addr is " << CRTP::single_net::instance() << std::endl;
    });

    t7.join();
    t8.join(); 
    t5.join();
    t6.join();
    t3.join();
    t4.join();
    t1.join();
    t2.join(); 

    return 0;
}