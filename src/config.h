#pragma once

// ----- WiFi -----
#define WIFI_SSID     "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
#define WIFI_TIMEOUT_MS 15000

// ----- Home Assistant -----
#define HA_BASE_URL "http://homeassistant.local:8123"
#define HA_TOKEN    "your_long_lived_access_token"

// ----- HA Entity IDs -----
#define HA_ENTITY_INDOOR_TEMP  "sensor.h5071_50bc_temperature"
#define HA_ENTITY_OUTDOOR_TEMP "sensor.xiamoi_t3_thermometer_temperature"
#define HA_ENTITY_WEATHER      "weather.forecast_home"
#define HA_ENTITY_SAUNA_TEMP   "climate.itc_308_wifi_thermostat"

// ----- Polling -----
#define HA_POLL_INTERVAL_MS 30000

// ----- Display -----
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480
#define LEFT_PANEL_W  220
#define RIGHT_PANEL_W (SCREEN_WIDTH - LEFT_PANEL_W)
#define STATUS_BAR_H  36
