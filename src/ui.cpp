#include "ui.h"
#include "config.h"
#include "weather_icons.h"
#include <lvgl.h>

// Accent colors (same in both themes)
#define COL_WARM      lv_color_hex(0xF97316)
#define COL_COLD      lv_color_hex(0x3B82F6)
#define COL_GREEN     lv_color_hex(0x22C55E)
#define COL_RED       lv_color_hex(0xEF4444)

// Theme-dependent colors
static bool dark_mode = true;

static lv_color_t col_bg()        { return lv_color_hex(dark_mode ? 0x0D1117 : 0xF0F2F5); }
static lv_color_t col_card()      { return lv_color_hex(dark_mode ? 0x21262D : 0xFFFFFF); }
static lv_color_t col_text()      { return lv_color_hex(dark_mode ? 0xFFFFFF : 0x1F2328); }
static lv_color_t col_text_dim()  { return lv_color_hex(dark_mode ? 0x8B949E : 0x656D76); }
static lv_color_t col_status_bar(){ return lv_color_hex(dark_mode ? 0x161B22 : 0xD0D7DE); }
static lv_color_t col_bar_track() { return lv_color_hex(dark_mode ? 0x30363D : 0xD0D7DE); }
static lv_color_t col_btn_bg()    { return lv_color_hex(dark_mode ? 0x30363D : 0xD0D7DE); }
static lv_color_t col_btn_border(){ return lv_color_hex(dark_mode ? 0x484F58 : 0xAFB8C1); }

// Status bar widgets
static lv_obj_t* lbl_wifi_icon   = nullptr;
static lv_obj_t* lbl_wifi_status = nullptr;
static lv_obj_t* lbl_title       = nullptr;
static lv_obj_t* lbl_updated     = nullptr;

// Left panel - indoor
static lv_obj_t* lbl_indoor_title = nullptr;
static lv_obj_t* bar_indoor       = nullptr;
static lv_obj_t* lbl_indoor_temp  = nullptr;

// Left panel - outdoor
static lv_obj_t* lbl_outdoor_title = nullptr;
static lv_obj_t* bar_outdoor       = nullptr;
static lv_obj_t* lbl_outdoor_temp  = nullptr;

// Left panel - sauna
static lv_obj_t* lbl_sauna_title = nullptr;
static lv_obj_t* bar_sauna       = nullptr;
static lv_obj_t* lbl_sauna_temp  = nullptr;

// Right panel - current weather
static lv_obj_t* lbl_weather_icon  = nullptr;
static lv_obj_t* lbl_weather_cond  = nullptr;
static lv_obj_t* lbl_weather_temp  = nullptr;
static lv_obj_t* lbl_weather_wind  = nullptr;
static lv_obj_t* lbl_weather_humid = nullptr;

// Right panel - forecast
static lv_obj_t* forecast_cards[3]      = {};
static lv_obj_t* lbl_fc_day[3]          = {};
static lv_obj_t* lbl_fc_icon[3]         = {};
static lv_obj_t* lbl_fc_cond[3]         = {};
static lv_obj_t* lbl_fc_high[3]         = {};
static lv_obj_t* lbl_fc_low[3]          = {};

// Containers (need refs for theme switching)
static lv_obj_t* obj_status_bar   = nullptr;
static lv_obj_t* obj_left_panel   = nullptr;
static lv_obj_t* obj_right_panel  = nullptr;
static lv_obj_t* obj_indoor_card  = nullptr;
static lv_obj_t* obj_outdoor_card = nullptr;
static lv_obj_t* obj_sauna_card   = nullptr;
static lv_obj_t* obj_current_card = nullptr;
static lv_obj_t* lbl_fc_title     = nullptr;

// Unit toggle
static lv_obj_t* btn_unit_toggle  = nullptr;
static lv_obj_t* lbl_unit_toggle  = nullptr;
static bool use_fahrenheit = false;
static HAWeatherData last_data = {};

// Theme toggle
static lv_obj_t* btn_theme_toggle = nullptr;
static lv_obj_t* lbl_theme_toggle = nullptr;

// Loading overlay
static lv_obj_t* loading_overlay = nullptr;
static lv_obj_t* loading_spinner = nullptr;
static lv_obj_t* loading_label   = nullptr;

// ----- Helper: card style -----
static void style_card(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, col_card(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, 8, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 10, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

// ----- Helper: invisible container -----
static void style_transparent(lv_obj_t* obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

// ----- Helper: C to F conversion -----
static float to_display_temp(float celsius) {
    if (use_fahrenheit) return celsius * 9.0f / 5.0f + 32.0f;
    return celsius;
}

static const char* temp_unit() {
    return use_fahrenheit ? "F" : "C";
}

// Forward declarations
void ui_update(const HAWeatherData& data);
static void apply_theme();

static void unit_toggle_cb(lv_event_t* e) {
    (void)e;
    use_fahrenheit = !use_fahrenheit;
    lv_label_set_text(lbl_unit_toggle, use_fahrenheit ? "\xC2\xB0" "F" : "\xC2\xB0" "C");
    if (last_data.has_data) {
        ui_update(last_data);
    }
}

static void theme_toggle_cb(lv_event_t* e) {
    (void)e;
    dark_mode = !dark_mode;
    lv_label_set_text(lbl_theme_toggle, dark_mode ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
    apply_theme();
}

// ----- Helper: thermometer color by temperature (always in Celsius) -----
static lv_color_t temp_color(float t) {
    if (t < 0)  return COL_COLD;
    if (t < 15) return lv_color_hex(0x06B6D4); // cyan
    if (t < 25) return COL_GREEN;
    if (t < 30) return COL_WARM;
    return COL_RED;
}

// ----- Apply theme colors to all widgets -----
static void apply_theme() {
    lv_color_t bg   = col_bg();
    lv_color_t card = col_card();
    lv_color_t text = col_text();
    lv_color_t dim  = col_text_dim();
    lv_color_t sbar = col_status_bar();
    lv_color_t trak = col_bar_track();
    lv_color_t bbg  = col_btn_bg();
    lv_color_t bbrd = col_btn_border();

    // Screen
    lv_obj_set_style_bg_color(lv_scr_act(), bg, 0);

    // Status bar
    lv_obj_set_style_bg_color(obj_status_bar, sbar, 0);
    lv_obj_set_style_text_color(lbl_title, text, 0);
    lv_obj_set_style_text_color(lbl_updated, dim, 0);

    // Buttons
    lv_obj_set_style_bg_color(btn_unit_toggle, bbg, 0);
    lv_obj_set_style_border_color(btn_unit_toggle, bbrd, 0);
    lv_obj_set_style_text_color(lbl_unit_toggle, text, 0);
    lv_obj_set_style_bg_color(btn_theme_toggle, bbg, 0);
    lv_obj_set_style_border_color(btn_theme_toggle, bbrd, 0);
    lv_obj_set_style_text_color(lbl_theme_toggle, text, 0);

    // Panels
    lv_obj_set_style_bg_color(obj_left_panel, bg, 0);
    lv_obj_set_style_bg_color(obj_right_panel, bg, 0);

    // Cards
    lv_obj_t* cards[] = {obj_indoor_card, obj_outdoor_card, obj_sauna_card, obj_current_card};
    for (auto c : cards) {
        lv_obj_set_style_bg_color(c, card, 0);
    }
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_bg_color(forecast_cards[i], card, 0);
    }

    // Titles (dim text)
    lv_obj_t* dim_labels[] = {lbl_indoor_title, lbl_outdoor_title, lbl_sauna_title,
                               lbl_weather_wind, lbl_weather_humid, lbl_fc_title};
    for (auto l : dim_labels) {
        lv_obj_set_style_text_color(l, dim, 0);
    }
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_text_color(lbl_fc_cond[i], dim, 0);
    }

    // Main text
    lv_obj_t* text_labels[] = {lbl_indoor_temp, lbl_outdoor_temp, lbl_sauna_temp,
                                lbl_weather_cond};
    for (auto l : text_labels) {
        lv_obj_set_style_text_color(l, text, 0);
    }
    for (int i = 0; i < 3; i++) {
        lv_obj_set_style_text_color(lbl_fc_day[i], text, 0);
    }

    // Bar track backgrounds
    lv_obj_t* bars[] = {bar_indoor, bar_outdoor, bar_sauna};
    for (auto b : bars) {
        lv_obj_set_style_bg_color(b, trak, 0);
    }

    // Loading overlay
    lv_obj_set_style_bg_color(loading_overlay, bg, 0);
    lv_obj_set_style_text_color(loading_label, text, 0);
    lv_obj_set_style_arc_color(loading_spinner, dim, 0);
}

// ----- Build the UI -----
void ui_create() {
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, col_bg(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // ===== STATUS BAR =====
    obj_status_bar = lv_obj_create(scr);
    lv_obj_set_size(obj_status_bar, SCREEN_WIDTH, STATUS_BAR_H);
    lv_obj_set_pos(obj_status_bar, 0, 0);
    lv_obj_set_style_bg_color(obj_status_bar, col_status_bar(), 0);
    lv_obj_set_style_bg_opa(obj_status_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj_status_bar, 0, 0);
    lv_obj_set_style_border_width(obj_status_bar, 0, 0);
    lv_obj_set_style_pad_hor(obj_status_bar, 10, 0);
    lv_obj_set_style_pad_ver(obj_status_bar, 0, 0);
    lv_obj_set_flex_flow(obj_status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj_status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(obj_status_bar, LV_OBJ_FLAG_SCROLLABLE);

    lbl_wifi_icon = lv_label_create(obj_status_bar);
    lv_label_set_text(lbl_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(lbl_wifi_icon, COL_GREEN, 0);
    lv_obj_set_style_text_font(lbl_wifi_icon, &lv_font_montserrat_16, 0);

    lbl_wifi_status = lv_label_create(obj_status_bar);
    lv_label_set_text(lbl_wifi_status, "Connecting...");
    lv_obj_set_style_text_color(lbl_wifi_status, col_text_dim(), 0);
    lv_obj_set_style_text_font(lbl_wifi_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_pad_left(lbl_wifi_status, 6, 0);

    lbl_title = lv_label_create(obj_status_bar);
    lv_label_set_text(lbl_title, "Home Weather");
    lv_obj_set_style_text_color(lbl_title, col_text(), 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_flex_grow(lbl_title, 1);
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);

    // °C / °F toggle button
    btn_unit_toggle = lv_btn_create(obj_status_bar);
    lv_obj_set_size(btn_unit_toggle, 44, 28);
    lv_obj_set_style_bg_color(btn_unit_toggle, col_btn_bg(), 0);
    lv_obj_set_style_bg_color(btn_unit_toggle, col_btn_border(), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_unit_toggle, 6, 0);
    lv_obj_set_style_border_width(btn_unit_toggle, 1, 0);
    lv_obj_set_style_border_color(btn_unit_toggle, col_btn_border(), 0);
    lv_obj_set_style_pad_all(btn_unit_toggle, 0, 0);
    lv_obj_set_style_shadow_width(btn_unit_toggle, 0, 0);
    lv_obj_add_event_cb(btn_unit_toggle, unit_toggle_cb, LV_EVENT_CLICKED, nullptr);

    lbl_unit_toggle = lv_label_create(btn_unit_toggle);
    lv_label_set_text(lbl_unit_toggle, "\xC2\xB0" "C");
    lv_obj_set_style_text_color(lbl_unit_toggle, col_text(), 0);
    lv_obj_set_style_text_font(lbl_unit_toggle, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_unit_toggle);

    // Light/Dark theme toggle button
    btn_theme_toggle = lv_btn_create(obj_status_bar);
    lv_obj_set_size(btn_theme_toggle, 40, 28);
    lv_obj_set_style_bg_color(btn_theme_toggle, col_btn_bg(), 0);
    lv_obj_set_style_bg_color(btn_theme_toggle, col_btn_border(), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_theme_toggle, 6, 0);
    lv_obj_set_style_border_width(btn_theme_toggle, 1, 0);
    lv_obj_set_style_border_color(btn_theme_toggle, col_btn_border(), 0);
    lv_obj_set_style_pad_all(btn_theme_toggle, 0, 0);
    lv_obj_set_style_shadow_width(btn_theme_toggle, 0, 0);
    lv_obj_add_event_cb(btn_theme_toggle, theme_toggle_cb, LV_EVENT_CLICKED, nullptr);

    lbl_theme_toggle = lv_label_create(btn_theme_toggle);
    lv_label_set_text(lbl_theme_toggle, LV_SYMBOL_EYE_CLOSE);
    lv_obj_set_style_text_color(lbl_theme_toggle, col_text(), 0);
    lv_obj_set_style_text_font(lbl_theme_toggle, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_theme_toggle);

    lbl_updated = lv_label_create(obj_status_bar);
    lv_label_set_text(lbl_updated, "Updated: --:--");
    lv_obj_set_style_text_color(lbl_updated, col_text_dim(), 0);
    lv_obj_set_style_text_font(lbl_updated, &lv_font_montserrat_12, 0);
    lv_obj_set_style_pad_left(lbl_updated, 8, 0);

    // ===== MAIN CONTENT AREA =====
    int content_y = STATUS_BAR_H;
    int content_h = SCREEN_HEIGHT - STATUS_BAR_H;

    // ===== LEFT PANEL (220px) =====
    obj_left_panel = lv_obj_create(scr);
    lv_obj_set_pos(obj_left_panel, 0, content_y);
    lv_obj_set_size(obj_left_panel, LEFT_PANEL_W, content_h);
    lv_obj_set_style_bg_color(obj_left_panel, col_bg(), 0);
    lv_obj_set_style_bg_opa(obj_left_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj_left_panel, 0, 0);
    lv_obj_set_style_border_width(obj_left_panel, 0, 0);
    lv_obj_set_style_pad_all(obj_left_panel, 6, 0);
    lv_obj_set_style_pad_row(obj_left_panel, 4, 0);
    lv_obj_set_flex_flow(obj_left_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj_left_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(obj_left_panel, LV_OBJ_FLAG_SCROLLABLE);

    // --- Indoor card ---
    obj_indoor_card = lv_obj_create(obj_left_panel);
    lv_obj_set_size(obj_indoor_card, lv_pct(100), LV_SIZE_CONTENT);
    style_card(obj_indoor_card);
    lv_obj_set_style_pad_all(obj_indoor_card, 8, 0);
    lv_obj_set_flex_flow(obj_indoor_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj_indoor_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(obj_indoor_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_indoor_title = lv_label_create(obj_indoor_card);
    lv_label_set_text(lbl_indoor_title, "INDOOR");
    lv_obj_set_style_text_color(lbl_indoor_title, col_text_dim(), 0);
    lv_obj_set_style_text_font(lbl_indoor_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_bottom(lbl_indoor_title, 2, 0);

    bar_indoor = lv_bar_create(obj_indoor_card);
    lv_obj_set_size(bar_indoor, 20, 40);
    lv_bar_set_range(bar_indoor, -10, 40);
    lv_bar_set_value(bar_indoor, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_indoor, col_bar_track(), 0);
    lv_obj_set_style_bg_color(bar_indoor, COL_WARM, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_indoor, 10, 0);
    lv_obj_set_style_radius(bar_indoor, 10, LV_PART_INDICATOR);

    lbl_indoor_temp = lv_label_create(obj_indoor_card);
    lv_label_set_text(lbl_indoor_temp, "--.- C");
    lv_obj_set_style_text_color(lbl_indoor_temp, col_text(), 0);
    lv_obj_set_style_text_font(lbl_indoor_temp, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_top(lbl_indoor_temp, 2, 0);

    // --- Outdoor card ---
    obj_outdoor_card = lv_obj_create(obj_left_panel);
    lv_obj_set_size(obj_outdoor_card, lv_pct(100), LV_SIZE_CONTENT);
    style_card(obj_outdoor_card);
    lv_obj_set_style_pad_all(obj_outdoor_card, 8, 0);
    lv_obj_set_flex_flow(obj_outdoor_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj_outdoor_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(obj_outdoor_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_outdoor_title = lv_label_create(obj_outdoor_card);
    lv_label_set_text(lbl_outdoor_title, "OUTDOOR");
    lv_obj_set_style_text_color(lbl_outdoor_title, col_text_dim(), 0);
    lv_obj_set_style_text_font(lbl_outdoor_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_bottom(lbl_outdoor_title, 2, 0);

    bar_outdoor = lv_bar_create(obj_outdoor_card);
    lv_obj_set_size(bar_outdoor, 20, 40);
    lv_bar_set_range(bar_outdoor, -20, 40);
    lv_bar_set_value(bar_outdoor, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_outdoor, col_bar_track(), 0);
    lv_obj_set_style_bg_color(bar_outdoor, COL_COLD, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_outdoor, 10, 0);
    lv_obj_set_style_radius(bar_outdoor, 10, LV_PART_INDICATOR);

    lbl_outdoor_temp = lv_label_create(obj_outdoor_card);
    lv_label_set_text(lbl_outdoor_temp, "--.- C");
    lv_obj_set_style_text_color(lbl_outdoor_temp, col_text(), 0);
    lv_obj_set_style_text_font(lbl_outdoor_temp, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_top(lbl_outdoor_temp, 2, 0);

    // --- Sauna card ---
    obj_sauna_card = lv_obj_create(obj_left_panel);
    lv_obj_set_size(obj_sauna_card, lv_pct(100), LV_SIZE_CONTENT);
    style_card(obj_sauna_card);
    lv_obj_set_style_pad_all(obj_sauna_card, 8, 0);
    lv_obj_set_flex_flow(obj_sauna_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj_sauna_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(obj_sauna_card, LV_OBJ_FLAG_SCROLLABLE);

    lbl_sauna_title = lv_label_create(obj_sauna_card);
    lv_label_set_text(lbl_sauna_title, "SAUNA");
    lv_obj_set_style_text_color(lbl_sauna_title, col_text_dim(), 0);
    lv_obj_set_style_text_font(lbl_sauna_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_bottom(lbl_sauna_title, 2, 0);

    bar_sauna = lv_bar_create(obj_sauna_card);
    lv_obj_set_size(bar_sauna, 20, 40);
    lv_bar_set_range(bar_sauna, 0, 110);
    lv_bar_set_value(bar_sauna, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_sauna, col_bar_track(), 0);
    lv_obj_set_style_bg_color(bar_sauna, COL_RED, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_sauna, 10, 0);
    lv_obj_set_style_radius(bar_sauna, 10, LV_PART_INDICATOR);

    lbl_sauna_temp = lv_label_create(obj_sauna_card);
    lv_label_set_text(lbl_sauna_temp, "--.- C");
    lv_obj_set_style_text_color(lbl_sauna_temp, col_text(), 0);
    lv_obj_set_style_text_font(lbl_sauna_temp, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_top(lbl_sauna_temp, 2, 0);

    // ===== RIGHT PANEL (580px) =====
    obj_right_panel = lv_obj_create(scr);
    lv_obj_set_pos(obj_right_panel, LEFT_PANEL_W, content_y);
    lv_obj_set_size(obj_right_panel, RIGHT_PANEL_W, content_h);
    lv_obj_set_style_bg_color(obj_right_panel, col_bg(), 0);
    lv_obj_set_style_bg_opa(obj_right_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj_right_panel, 0, 0);
    lv_obj_set_style_border_width(obj_right_panel, 0, 0);
    lv_obj_set_style_pad_all(obj_right_panel, 8, 0);
    lv_obj_set_flex_flow(obj_right_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(obj_right_panel, LV_OBJ_FLAG_SCROLLABLE);

    // --- Current weather card ---
    obj_current_card = lv_obj_create(obj_right_panel);
    lv_obj_set_size(obj_current_card, lv_pct(100), LV_SIZE_CONTENT);
    style_card(obj_current_card);
    lv_obj_set_style_pad_all(obj_current_card, 16, 0);
    lv_obj_clear_flag(obj_current_card, LV_OBJ_FLAG_SCROLLABLE);

    // Top row: icon + condition + temperature
    lv_obj_t* weather_row = lv_obj_create(obj_current_card);
    lv_obj_set_size(weather_row, lv_pct(100), LV_SIZE_CONTENT);
    style_transparent(weather_row);
    lv_obj_set_flex_flow(weather_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(weather_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lbl_weather_icon = lv_label_create(weather_row);
    lv_label_set_text(lbl_weather_icon, ICON_WEATHER_SUNNY);
    lv_obj_set_style_text_color(lbl_weather_icon, lv_color_hex(0xFBBF24), 0);
    lv_obj_set_style_text_font(lbl_weather_icon, &weather_font_40, 0);

    lbl_weather_cond = lv_label_create(weather_row);
    lv_label_set_text(lbl_weather_cond, "Loading...");
    lv_obj_set_style_text_color(lbl_weather_cond, col_text(), 0);
    lv_obj_set_style_text_font(lbl_weather_cond, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_left(lbl_weather_cond, 12, 0);

    lbl_weather_temp = lv_label_create(weather_row);
    lv_label_set_text(lbl_weather_temp, "-- C");
    lv_obj_set_style_text_color(lbl_weather_temp, col_text(), 0);
    lv_obj_set_style_text_font(lbl_weather_temp, &lv_font_montserrat_40, 0);
    lv_obj_set_flex_grow(lbl_weather_temp, 1);
    lv_obj_set_style_text_align(lbl_weather_temp, LV_TEXT_ALIGN_RIGHT, 0);

    // Details row: wind + humidity
    lv_obj_t* details_row = lv_obj_create(obj_current_card);
    lv_obj_set_size(details_row, lv_pct(100), LV_SIZE_CONTENT);
    style_transparent(details_row);
    lv_obj_set_style_pad_top(details_row, 8, 0);
    lv_obj_set_flex_flow(details_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(details_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lbl_weather_wind = lv_label_create(details_row);
    lv_label_set_text(lbl_weather_wind, "Wind: -- km/h");
    lv_obj_set_style_text_color(lbl_weather_wind, col_text_dim(), 0);
    lv_obj_set_style_text_font(lbl_weather_wind, &lv_font_montserrat_16, 0);

    lv_obj_t* detail_spacer = lv_obj_create(details_row);
    lv_obj_set_size(detail_spacer, 30, 1);
    style_transparent(detail_spacer);

    lbl_weather_humid = lv_label_create(details_row);
    lv_label_set_text(lbl_weather_humid, "Humidity: --%");
    lv_obj_set_style_text_color(lbl_weather_humid, col_text_dim(), 0);
    lv_obj_set_style_text_font(lbl_weather_humid, &lv_font_montserrat_16, 0);

    // Spacer between current and forecast
    lv_obj_t* mid_spacer = lv_obj_create(obj_right_panel);
    lv_obj_set_size(mid_spacer, 1, 8);
    style_transparent(mid_spacer);

    // --- Forecast section ---
    lbl_fc_title = lv_label_create(obj_right_panel);
    lv_label_set_text(lbl_fc_title, "3-DAY FORECAST");
    lv_obj_set_style_text_color(lbl_fc_title, col_text_dim(), 0);
    lv_obj_set_style_text_font(lbl_fc_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_pad_bottom(lbl_fc_title, 6, 0);

    // Forecast row
    lv_obj_t* fc_row = lv_obj_create(obj_right_panel);
    lv_obj_set_size(fc_row, lv_pct(100), LV_SIZE_CONTENT);
    style_transparent(fc_row);
    lv_obj_set_flex_flow(fc_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(fc_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    for (int i = 0; i < 3; i++) {
        forecast_cards[i] = lv_obj_create(fc_row);
        lv_obj_set_size(forecast_cards[i], 160, LV_SIZE_CONTENT);
        style_card(forecast_cards[i]);
        lv_obj_set_style_pad_all(forecast_cards[i], 12, 0);
        lv_obj_set_flex_flow(forecast_cards[i], LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(forecast_cards[i], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(forecast_cards[i], LV_OBJ_FLAG_SCROLLABLE);

        lbl_fc_day[i] = lv_label_create(forecast_cards[i]);
        lv_label_set_text(lbl_fc_day[i], "---");
        lv_obj_set_style_text_color(lbl_fc_day[i], col_text(), 0);
        lv_obj_set_style_text_font(lbl_fc_day[i], &lv_font_montserrat_16, 0);
        lv_obj_set_style_pad_bottom(lbl_fc_day[i], 4, 0);

        lbl_fc_icon[i] = lv_label_create(forecast_cards[i]);
        lv_label_set_text(lbl_fc_icon[i], ICON_WEATHER_CLOUDY);
        lv_obj_set_style_text_color(lbl_fc_icon[i], lv_color_hex(0xFBBF24), 0);
        lv_obj_set_style_text_font(lbl_fc_icon[i], &weather_font_24, 0);
        lv_obj_set_style_pad_bottom(lbl_fc_icon[i], 4, 0);

        lbl_fc_cond[i] = lv_label_create(forecast_cards[i]);
        lv_label_set_text(lbl_fc_cond[i], "--");
        lv_obj_set_style_text_color(lbl_fc_cond[i], col_text_dim(), 0);
        lv_obj_set_style_text_font(lbl_fc_cond[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_pad_bottom(lbl_fc_cond[i], 2, 0);

        lbl_fc_high[i] = lv_label_create(forecast_cards[i]);
        lv_label_set_text(lbl_fc_high[i], "H: --");
        lv_obj_set_style_text_color(lbl_fc_high[i], COL_WARM, 0);
        lv_obj_set_style_text_font(lbl_fc_high[i], &lv_font_montserrat_14, 0);

        lbl_fc_low[i] = lv_label_create(forecast_cards[i]);
        lv_label_set_text(lbl_fc_low[i], "L: --");
        lv_obj_set_style_text_color(lbl_fc_low[i], COL_COLD, 0);
        lv_obj_set_style_text_font(lbl_fc_low[i], &lv_font_montserrat_14, 0);
    }

    // ===== LOADING OVERLAY =====
    loading_overlay = lv_obj_create(scr);
    lv_obj_set_size(loading_overlay, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(loading_overlay, 0, 0);
    lv_obj_set_style_bg_color(loading_overlay, col_bg(), 0);
    lv_obj_set_style_bg_opa(loading_overlay, LV_OPA_80, 0);
    lv_obj_set_style_radius(loading_overlay, 0, 0);
    lv_obj_set_style_border_width(loading_overlay, 0, 0);
    lv_obj_clear_flag(loading_overlay, LV_OBJ_FLAG_SCROLLABLE);

    loading_spinner = lv_spinner_create(loading_overlay, 1000, 60);
    lv_obj_set_size(loading_spinner, 60, 60);
    lv_obj_center(loading_spinner);
    lv_obj_set_style_arc_color(loading_spinner, col_text_dim(), 0);
    lv_obj_set_style_arc_color(loading_spinner, COL_GREEN, LV_PART_INDICATOR);

    loading_label = lv_label_create(loading_overlay);
    lv_label_set_text(loading_label, "Connecting...");
    lv_obj_set_style_text_color(loading_label, col_text(), 0);
    lv_obj_set_style_text_font(loading_label, &lv_font_montserrat_16, 0);
    lv_obj_align_to(loading_label, loading_spinner, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);
}

// ----- Update UI with new data -----
void ui_update(const HAWeatherData& data) {
    last_data = data;
    char buf[64];
    const char* u = temp_unit();

    // Indoor temperature
    if (data.indoor_temp.valid) {
        snprintf(buf, sizeof(buf), "%.1f\xC2\xB0%s", to_display_temp(data.indoor_temp.value), u);
        lv_label_set_text(lbl_indoor_temp, buf);
        lv_bar_set_value(bar_indoor, (int)data.indoor_temp.value, LV_ANIM_ON);
        lv_obj_set_style_bg_color(bar_indoor, temp_color(data.indoor_temp.value), LV_PART_INDICATOR);
    }

    // Outdoor temperature
    if (data.outdoor_temp.valid) {
        snprintf(buf, sizeof(buf), "%.1f\xC2\xB0%s", to_display_temp(data.outdoor_temp.value), u);
        lv_label_set_text(lbl_outdoor_temp, buf);
        lv_bar_set_value(bar_outdoor, (int)data.outdoor_temp.value, LV_ANIM_ON);
        lv_obj_set_style_bg_color(bar_outdoor, temp_color(data.outdoor_temp.value), LV_PART_INDICATOR);
    }

    // Sauna temperature
    if (data.sauna_temp.valid) {
        snprintf(buf, sizeof(buf), "%.1f\xC2\xB0%s", to_display_temp(data.sauna_temp.value), u);
        lv_label_set_text(lbl_sauna_temp, buf);
        lv_bar_set_value(bar_sauna, (int)data.sauna_temp.value, LV_ANIM_ON);
        lv_color_t sc;
        if (data.sauna_temp.value >= 60) sc = COL_RED;
        else if (data.sauna_temp.value >= 30) sc = COL_WARM;
        else sc = lv_color_hex(0x06B6D4);
        lv_obj_set_style_bg_color(bar_sauna, sc, LV_PART_INDICATOR);
    }

    // Current weather
    if (data.current.valid) {
        WeatherDisplay wd = weather_get_display(data.current.condition.c_str());
        lv_label_set_text(lbl_weather_icon, wd.icon);
        lv_label_set_text(lbl_weather_cond, wd.label);

        snprintf(buf, sizeof(buf), "%.0f\xC2\xB0%s", to_display_temp(data.current.temperature), u);
        lv_label_set_text(lbl_weather_temp, buf);
        lv_obj_set_style_text_color(lbl_weather_temp, temp_color(data.current.temperature), 0);

        snprintf(buf, sizeof(buf), "Wind: %.0f km/h", data.current.wind_speed);
        lv_label_set_text(lbl_weather_wind, buf);

        snprintf(buf, sizeof(buf), "Humidity: %.0f%%", data.current.humidity);
        lv_label_set_text(lbl_weather_humid, buf);
    }

    // Forecast
    for (int i = 0; i < 3; i++) {
        if (data.forecast[i].valid) {
            lv_label_set_text(lbl_fc_day[i], data.forecast[i].day_name.c_str());

            WeatherDisplay wd = weather_get_display(data.forecast[i].condition.c_str());
            lv_label_set_text(lbl_fc_icon[i], wd.icon);
            lv_label_set_text(lbl_fc_cond[i], wd.label);

            snprintf(buf, sizeof(buf), "H: %.0f\xC2\xB0", to_display_temp(data.forecast[i].temp_high));
            lv_label_set_text(lbl_fc_high[i], buf);

            snprintf(buf, sizeof(buf), "L: %.0f\xC2\xB0", to_display_temp(data.forecast[i].temp_low));
            lv_label_set_text(lbl_fc_low[i], buf);
        }
    }

    // Updated timestamp
    if (data.last_updated.length() > 0) {
        snprintf(buf, sizeof(buf), "Updated: %s", data.last_updated.c_str());
        lv_label_set_text(lbl_updated, buf);
    }
}

void ui_show_loading(bool show) {
    if (loading_overlay) {
        if (show) {
            lv_obj_clear_flag(loading_overlay, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(loading_overlay, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_set_wifi_status(bool connected) {
    if (connected) {
        lv_label_set_text(lbl_wifi_icon, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(lbl_wifi_icon, COL_GREEN, 0);
        lv_label_set_text(lbl_wifi_status, "Connected");
        lv_obj_set_style_text_color(lbl_wifi_status, COL_GREEN, 0);
    } else {
        lv_label_set_text(lbl_wifi_icon, LV_SYMBOL_WARNING);
        lv_obj_set_style_text_color(lbl_wifi_icon, COL_RED, 0);
        lv_label_set_text(lbl_wifi_status, "Disconnected");
        lv_obj_set_style_text_color(lbl_wifi_status, COL_RED, 0);
    }
}
