#include <iostream>
#include <vector>  
#include <algorithm> 
#include <memory>
#include <thread>

// typedef int (*funcp)(int, int);

using funcp = int(*)(int, int);

int add(int a, int b)
{
    return a + b;
}

// ---

struct _adder
{
    int __to_add;
    _adder(int val) : __to_add(val) {}
    ~_adder()
    {
        std::cout << "Destructor called for _adder with value: " << __to_add << std::endl;
    }
    // Functors
    int operator() (int x)
    {
        return x + __to_add;
    }

    void add(int x)
    {
        __to_add += x;
    }
    
}; 

struct is_great_than
{
    int _threshold;
    is_great_than(int threshold) : _threshold(threshold) {}
    bool operator() (int x)
    {
        return x > _threshold;
    }
};

template<typename T>
struct cmp
{
    bool operator() (const T& a, const T& b) const
    {
        return a < b;
    }
};


int main()
{
    funcp func;
    func = &add;
    
    std::cout << func(1, 2) << std::endl;

    _adder adder(5);
    // Functors
    std::cout << adder(10) << std::endl;

    std::vector<int> vec{1, 2, 3, 4, 5, 6, 7, 8, 9};
    int x = 5;
    auto it = std::find_if(vec.begin(), vec.end(), is_great_than(x));

    if (it != vec.end())
    {
        std::cout << "First element greater than " << x << " is: " << *it << std::endl;
    }

    std::vector<int> vec2{9, 8, 7, 6, 5, 4, 3, 2, 1};
    std::sort(vec.begin(), vec.end(), cmp<int>());

    for (auto t: vec)
    {
        std::cout << t << " ";
    }
    std::cout << std::endl;

    // lambda
    int threshold = 5;
    std::vector<int> v3 = {1, 6, 3, 8, 2, 7, 4, 9, 5};
    auto new_end = std::remove_if(v3.begin(), v3.end(), 
        [threshold](int x) {
            return x < threshold;
        });
    v3.erase(new_end, v3.end());

    for (auto t: v3)
    {
        std::cout << t << " ";
    }
    std::cout << std::endl;

    int tmp = 2;
    auto lambda = [&tmp](int x){
        return tmp *= x;
    };

    lambda(3);
    std::cout << "tmp: " << tmp << std::endl;
    
    std::thread t;
    {
        auto add_p = std::make_shared<_adder>(10); 
        auto lambda2 = [add_p](int x){
            std::this_thread::sleep_for(std::chrono::seconds(1));
            add_p->add(x);
            std::cout << add_p.use_count() << std::endl;
        };
        // lambda2(5); 
        t = std::thread(lambda2, 5);
    }

    t.join();
    
    return 0;
}