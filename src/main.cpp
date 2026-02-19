#include <Arduino.h>
#include <lvgl.h>

#include "config.h"
#include "display.h"
#include "touch.h"
#include "wifi_manager.h"
#include "ha_client.h"
#include "ui.h"

static HAWeatherData weather_data;
static bool first_fetch_done = false;

static void ha_poll_cb(lv_timer_t* timer) {
    // Check WiFi and update status
    wifi_check_reconnect();
    bool connected = wifi_is_connected();
    ui_set_wifi_status(connected);

    if (!connected) return;

    Serial.println("Fetching HA data...");
    ha_fetch_all(weather_data);
    ui_update(weather_data);

    if (!first_fetch_done) {
        first_fetch_done = true;
        ui_show_loading(false);
        Serial.println("First data loaded - hiding loading screen");
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Home Weather Dashboard ===");

    // Initialize display + LVGL
    display_init();
    lv_disp_t* disp = display_get();

    // Initialize touch
    touch_init(disp);

    // Build UI (shows loading overlay initially)
    ui_create();
    Serial.println("UI created");

    // Initialize WiFi
    wifi_init();
    ui_set_wifi_status(wifi_is_connected());

    // Initialize HA client
    ha_client_init();

    // Configure NTP for timestamps
    configTime(0, 0, "pool.ntp.org");

    // Create LVGL timer for HA polling every 30 seconds
    // First poll happens immediately (delay=0 triggers on first tick)
    lv_timer_t* poll_timer = lv_timer_create(ha_poll_cb, HA_POLL_INTERVAL_MS, nullptr);
    lv_timer_ready(poll_timer); // trigger immediately on first loop iteration

    Serial.println("Setup complete");
}

void loop() {
    lv_timer_handler();
    delay(5);
}
