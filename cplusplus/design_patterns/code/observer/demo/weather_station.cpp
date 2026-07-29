#include "weather_station.hpp"

// ============ WeatherStation ============
void WeatherStation::SetMeasurements(double temp, double hum) {
    temperature_ = temp;
    humidity_ = hum;
    std::cout << "\n[气象站] 新数据: "
              << temperature_ << "°C, " << humidity_ << "%"
              << std::endl;
    Notify();
}

double WeatherStation::GetTemperature() const { return temperature_; }
double WeatherStation::GetHumidity()    const { return humidity_; }

// ============ PhoneDisplay ============
PhoneDisplay::PhoneDisplay(const std::string& name, WeatherStation& station)
    : name_(name), station_(station) {}

void PhoneDisplay::Update() {
    std::cout << "  [" << name_ << " 手机] "
              << station_.GetTemperature() << "°C, "
              << station_.GetHumidity() << "%"
              << std::endl;
}

// ============ WebDisplay ============
WebDisplay::WebDisplay(WeatherStation& station) : station_(station) {}

void WebDisplay::Update() {
    std::cout << "  [网页后台] "
              << station_.GetTemperature() << "°C"
              << std::endl;
}
