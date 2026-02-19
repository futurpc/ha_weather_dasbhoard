#include "ha_client.h"
#include "config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const char* TAG = "HA";

static const char* DAY_NAMES[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

void ha_client_init() {
    // Nothing to initialize
}

static bool ha_get(const String& url, JsonDocument& doc) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + HA_TOKEN);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);

    int code = http.GET();
    if (code != 200) {
        Serial.printf("[%s] GET %s failed: %d\n", TAG, url.c_str(), code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[%s] JSON parse error: %s\n", TAG, err.c_str());
        return false;
    }
    return true;
}

static bool ha_post(const String& url, const String& body, JsonDocument& doc) {
    HTTPClient http;
    http.begin(url);
    http.addHeader("Authorization", String("Bearer ") + HA_TOKEN);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);

    int code = http.POST(body);
    if (code != 200) {
        Serial.printf("[%s] POST %s failed: %d\n", TAG, url.c_str(), code);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[%s] JSON parse error: %s\n", TAG, err.c_str());
        return false;
    }
    return true;
}

static void fetch_temperature(const char* entity_id, HATemperature& temp) {
    String url = String(HA_BASE_URL) + "/api/states/" + entity_id;
    JsonDocument doc;

    if (!ha_get(url, doc)) {
        temp.valid = false;
        return;
    }

    const char* state = doc["state"];
    if (state && strcmp(state, "unavailable") != 0 && strcmp(state, "unknown") != 0) {
        temp.value = atof(state);
        temp.valid = true;
        Serial.printf("[%s] %s = %.1f\n", TAG, entity_id, temp.value);
    } else {
        temp.valid = false;
    }
}

static void fetch_climate_temperature(const char* entity_id, HATemperature& temp) {
    String url = String(HA_BASE_URL) + "/api/states/" + entity_id;
    JsonDocument doc;

    if (!ha_get(url, doc)) {
        temp.valid = false;
        return;
    }

    const char* state = doc["state"];
    if (state && strcmp(state, "unavailable") != 0 && strcmp(state, "unknown") != 0) {
        // Climate entities store current temp in attributes, not state
        float val = doc["attributes"]["current_temperature"] | -999.0f;
        if (val > -999.0f) {
            temp.value = val;
            temp.valid = true;
            Serial.printf("[%s] %s = %.1f\n", TAG, entity_id, temp.value);
        } else {
            temp.valid = false;
        }
    } else {
        temp.valid = false;
    }
}

static void fetch_current_weather(HACurrentWeather& weather) {
    String url = String(HA_BASE_URL) + "/api/states/" + HA_ENTITY_WEATHER;
    JsonDocument doc;

    if (!ha_get(url, doc)) {
        weather.valid = false;
        return;
    }

    weather.condition   = doc["state"].as<String>();
    weather.temperature = doc["attributes"]["temperature"] | 0.0f;
    weather.humidity    = doc["attributes"]["humidity"] | 0.0f;
    weather.wind_speed  = doc["attributes"]["wind_speed"] | 0.0f;
    weather.valid       = true;

    Serial.printf("[%s] Weather: %s %.1f°C\n", TAG, weather.condition.c_str(), weather.temperature);
}

static void parse_forecast_array(JsonArray fc, HAForecastDay forecast[3]) {
    for (int i = 0; i < 3 && i < (int)fc.size(); i++) {
        JsonObject day = fc[i];
        forecast[i].condition = day["condition"].as<String>();
        forecast[i].temp_high = day["temperature"] | 0.0f;
        forecast[i].temp_low  = day["templow"] | 0.0f;

        // Parse day name from datetime string "2024-01-15T..."
        const char* dt = day["datetime"];
        if (dt) {
            struct tm tm_val = {};
            int y, m, d;
            if (sscanf(dt, "%d-%d-%d", &y, &m, &d) == 3) {
                tm_val.tm_year = y - 1900;
                tm_val.tm_mon  = m - 1;
                tm_val.tm_mday = d;
                mktime(&tm_val);
                forecast[i].day_name = DAY_NAMES[tm_val.tm_wday];
            } else {
                forecast[i].day_name = "???";
            }
        } else {
            forecast[i].day_name = "???";
        }

        forecast[i].valid = true;
        Serial.printf("[%s] Forecast %s: %s H:%.0f L:%.0f\n",
                      TAG, forecast[i].day_name.c_str(),
                      forecast[i].condition.c_str(),
                      forecast[i].temp_high, forecast[i].temp_low);
    }
}

static void fetch_forecast(HAForecastDay forecast[3]) {
    for (int i = 0; i < 3; i++) forecast[i].valid = false;

    // Method 1: Try reading forecast from weather entity attributes (older HA / some integrations)
    {
        String url = String(HA_BASE_URL) + "/api/states/" + HA_ENTITY_WEATHER;
        JsonDocument doc;
        if (ha_get(url, doc)) {
            JsonArray fc = doc["attributes"]["forecast"];
            if (!fc.isNull() && fc.size() > 0) {
                Serial.printf("[%s] Forecast from entity attributes\n", TAG);
                parse_forecast_array(fc, forecast);
                return;
            }
        }
    }

    // Method 2: Try service call with return_response (HA 2024.7+)
    {
        String url = String(HA_BASE_URL) + "/api/services/weather/get_forecasts?return_response";
        String body = "{\"entity_id\":\"" + String(HA_ENTITY_WEATHER) + "\",\"type\":\"daily\"}";
        JsonDocument doc;

        if (!ha_post(url, body, doc)) {
            Serial.printf("[%s] Forecast service call failed\n", TAG);
            return;
        }

        // Try response format: { "weather.xxx": { "forecast": [...] } }
        JsonArray fc = doc[HA_ENTITY_WEATHER]["forecast"];
        if (fc.isNull()) {
            // Try wrapped: { "service_response": { "weather.xxx": { "forecast": [...] } } }
            fc = doc["service_response"][HA_ENTITY_WEATHER]["forecast"];
        }
        if (fc.isNull()) {
            Serial.printf("[%s] No forecast array in service response\n", TAG);
            // Debug: print raw response
            String raw;
            serializeJson(doc, raw);
            Serial.printf("[%s] Raw: %.200s\n", TAG, raw.c_str());
            return;
        }

        Serial.printf("[%s] Forecast from service call\n", TAG);
        parse_forecast_array(fc, forecast);
    }
}

void ha_fetch_all(HAWeatherData& data) {
    fetch_temperature(HA_ENTITY_INDOOR_TEMP, data.indoor_temp);
    fetch_temperature(HA_ENTITY_OUTDOOR_TEMP, data.outdoor_temp);
    fetch_climate_temperature(HA_ENTITY_SAUNA_TEMP, data.sauna_temp);
    fetch_current_weather(data.current);
    fetch_forecast(data.forecast);

    // Timestamp
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 100)) {
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        data.last_updated = buf;
    } else {
        unsigned long s = millis() / 1000;
        char buf[10];
        snprintf(buf, sizeof(buf), "%lum%lus", s / 60, s % 60);
        data.last_updated = buf;
    }
    data.has_data = true;
}
