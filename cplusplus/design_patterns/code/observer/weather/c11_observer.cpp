#include <functional>
#include <iostream>
#include <vector>

class EventEmitter
{
public:
    void on_update(std::function<void(double, double)> cb)
    {
        cbs_.push_back(std::move(cb));
    }

    void notify(double t, double h)
    {
        for (auto &cb : cbs_)
            cb(t, h);
    }

private:
    std::vector<std::function<void(double, double)>> cbs_;
};

int main()
{
    EventEmitter station;

    station.on_update(
        [](double t, double h) -> void
        { std::cout << "[手机] " << t << "°C, " << h << "%" << std::endl; });

    station.on_update([](double temp, double hum)
                      { std::cout << "[Web] " << temp << "°C" << std::endl; });

    station.notify(35.5, 65.0);
    station.notify(40.1, 60.3);
    
    return 0;
}