#pragma once
#include <Arduino.h>

struct HATemperature {
    float value;
    bool  valid;
};

struct HACurrentWeather {
    String condition;   // e.g. "partly-cloudy", "rainy"
    float  temperature;
    float  humidity;
    float  wind_speed;
    bool   valid;
};

struct HAForecastDay {
    String day_name;    // e.g. "Wed"
    String condition;
    float  temp_high;
    float  temp_low;
    bool   valid;
};

struct HAWeatherData {
    HATemperature    indoor_temp;
    HATemperature    outdoor_temp;
    HATemperature    sauna_temp;
    HACurrentWeather current;
    HAForecastDay    forecast[3];
    String           last_updated;  // "HH:MM"
    bool             has_data;
};

void ha_client_init();
void ha_fetch_all(HAWeatherData& data);
