#pragma once
#include "ha_client.h"

void ui_create();
void ui_update(const HAWeatherData& data);
void ui_show_loading(bool show);
void ui_set_wifi_status(bool connected);
