#pragma once

#ifndef SIMULATOR
#include "secrets.h"  // WIFI_SSID, WIFI_PASSWORD, HA_BASE_URL, HA_TOKEN
#endif

// ----- WiFi -----
#define WIFI_TIMEOUT_MS 15000

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
