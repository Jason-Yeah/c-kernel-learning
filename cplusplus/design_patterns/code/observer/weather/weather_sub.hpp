#pragma

#include "observer.hpp"
#include "subject.hpp"
#include <iostream>
#include <string>

class WeatherStation : public Subject
{
    double temperature_ = 0;
    double humidity_ = 0;

public:
    void SetMeasurements(double temp, double hum)
    {
        temperature_ = temp;
        humidity_ = hum;
        std::cout << "\n[气象站] 新数据: " << temperature_ << "°C, "
                  << humidity_ << "%" << std::endl;
        Notify();
    }
    double GetTemperature() const { return temperature_; }

    double GetHumidity() const { return humidity_; }
};

// ============ 具体观察者：手机端 ============
class PhoneDisplay : public Observer
{
    std::string name_;
    WeatherStation &station_;

public:
    PhoneDisplay(const std::string &name, WeatherStation &station)
        : name_(name), station_(station)
    {
    }

    void update() override
    {
        std::cout << "  [" << name_ << " 手机] " << station_.GetTemperature()
                  << "°C, " << station_.GetHumidity() << "%" << std::endl;
    }
};

// ============ 具体观察者：Web 端 ============
class WebDisplay : public Observer
{
    WeatherStation &station_;

public:
    explicit WebDisplay(WeatherStation &station) : station_(station) {}
    void update() override
    {
        std::cout << "  [网页后台] " << station_.GetTemperature() << "°C"
                  << std::endl;
    }
};

void Subject::Notify()
{
    for (auto *obs : observers_)
        obs->update(); // ← 此时 Observer 已经完整定义了，可以调 Update()
}
