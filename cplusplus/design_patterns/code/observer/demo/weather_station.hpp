#pragma once

#include "subject.hpp"
#include "observer.hpp"   // PhoneDisplay 要继承 Observer，必须看到完整定义
#include <string>
#include <iostream>

// ============ 具体被观察者：气象站 ============
class WeatherStation : public Subject {
    double temperature_ = 0;
    double humidity_ = 0;

public:
    void SetMeasurements(double temp, double hum);
    double GetTemperature() const;
    double GetHumidity()    const;
};

// ============ 具体观察者：手机端 ============
class PhoneDisplay : public Observer {
    std::string name_;
    WeatherStation& station_;

public:
    PhoneDisplay(const std::string& name, WeatherStation& station);
    void Update() override;
};

// ============ 具体观察者：Web 端 ============
class WebDisplay : public Observer {
    WeatherStation& station_;

public:
    explicit WebDisplay(WeatherStation& station);
    void Update() override;
};
