#include "weather_station.hpp"

int main() {
    WeatherStation station;

    PhoneDisplay phone("张三", station);
    WebDisplay web(station);

    station.Attach(&phone);
    station.Attach(&web);
    station.SetMeasurements(25.5, 65.0);
    station.SetMeasurements(26.1, 60.3);
    station.Detach(&web);
    station.SetMeasurements(27.0, 55.0);

    return 0;
}
