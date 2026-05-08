#include <iostream>
#include <vector>
#include <functional>

// 返回是void,参数一个是int
using callback = std::function<void(int)>;

class EventSystem
{
    private:
        std::vector<callback> callbacks;
    public:
        void register_callback(const callback &cb)
        {
            callbacks.push_back(cb);
        }

        void trigger_event(int data)
        {
            std::cout << "Event triggered with data = " << data 
            << ". Executing callbacks..." << std::endl;
            // 引用是为了减少开销，不拷贝了
            for (const auto &cb: callbacks)
                cb(data);
        }
};

void on_event(int data) {
    std::cout << "Function callback received data: " << data << std::endl;
}

int main()
{
    EventSystem eventsys;

    eventsys.register_callback(on_event);
    eventsys.register_callback([](int x) {
        std::cout << "Lambda callback received: " << x * 2 << std::endl;
    });

    int n = 5;
    eventsys.register_callback([n](int x){
        printf("捕获的 n = %d, 传入的 x = %d\n", n, x);
    });

    int data = 10;
    eventsys.trigger_event(data);

    return 0;
}