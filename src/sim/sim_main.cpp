#ifdef SIMULATOR

#include <lvgl.h>
#include <sdl/sdl.h>   // lv_drivers SDL

#include <unistd.h>
#include <cstdio>

#include "../ui.h"

// SDL driver exposes this flag
extern volatile bool sdl_quit_qry;

static HAWeatherData make_mock_data() {
    HAWeatherData d{};
    d.has_data = true;
    d.last_updated = "14:32";

    d.indoor_temp  = {22.4f, true};
    d.outdoor_temp = {-2.1f, true};
    d.sauna_temp   = {68.5f, true};

    d.current.condition   = "partlycloudy";
    d.current.temperature = 5.0f;
    d.current.humidity    = 72.0f;
    d.current.wind_speed  = 14.0f;
    d.current.valid       = true;

    d.forecast[0] = {"Wed", "cloudy",    8.0f, 2.0f, true};
    d.forecast[1] = {"Thu", "rainy",    12.0f, 5.0f, true};
    d.forecast[2] = {"Fri", "sunny",    18.0f, 9.0f, true};

    return d;
}

int main(int /*argc*/, char** /*argv*/) {
    lv_init();

    // Initialize SDL display via lv_drivers
    sdl_init();

    // --- Display driver ---
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[800 * 100];
    lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, 800 * 100);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = 800;
    disp_drv.ver_res  = 480;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_t* disp = lv_disp_drv_register(&disp_drv);

    // Dark theme
    lv_theme_t* theme = lv_theme_default_init(
        disp,
        lv_color_hex(0x3B82F6),
        lv_color_hex(0xEF4444),
        true,
        LV_FONT_DEFAULT
    );
    lv_disp_set_theme(disp, theme);

    // --- Mouse input (SDL provides this) ---
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);

    // --- Build UI ---
    ui_create();
    ui_set_wifi_status(true);
    ui_show_loading(false);

    HAWeatherData data = make_mock_data();
    ui_update(data);

    printf("Simulator running — close window to exit\n");

    // --- Main loop ---
    while (!sdl_quit_qry) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

#endif // SIMULATOR
