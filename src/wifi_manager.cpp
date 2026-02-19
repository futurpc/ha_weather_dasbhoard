#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

static unsigned long last_reconnect_attempt = 0;
static const unsigned long RECONNECT_INTERVAL_MS = 10000;

void wifi_init() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected! IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("WiFi connection failed - will retry");
    }
}

bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

void wifi_check_reconnect() {
    if (WiFi.status() == WL_CONNECTED) return;

    unsigned long now = millis();
    if (now - last_reconnect_attempt < RECONNECT_INTERVAL_MS) return;

    last_reconnect_attempt = now;
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
