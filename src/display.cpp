#include "display.h"
#include "config.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_RGB _panel_instance;
    lgfx::Bus_RGB   _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX() {
        // Backlight configuration
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = GPIO_NUM_2;
            cfg.invert = false;
            cfg.freq   = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        // Bus configuration for CrowPanel 7.0"
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;
            cfg.pin_d0  = GPIO_NUM_15; // B0
            cfg.pin_d1  = GPIO_NUM_7;  // B1
            cfg.pin_d2  = GPIO_NUM_6;  // B2
            cfg.pin_d3  = GPIO_NUM_5;  // B3
            cfg.pin_d4  = GPIO_NUM_4;  // B4
            cfg.pin_d5  = GPIO_NUM_9;  // G0
            cfg.pin_d6  = GPIO_NUM_46; // G1
            cfg.pin_d7  = GPIO_NUM_3;  // G2
            cfg.pin_d8  = GPIO_NUM_8;  // G3
            cfg.pin_d9  = GPIO_NUM_16; // G4
            cfg.pin_d10 = GPIO_NUM_1;  // G5
            cfg.pin_d11 = GPIO_NUM_14; // R0
            cfg.pin_d12 = GPIO_NUM_21; // R1
            cfg.pin_d13 = GPIO_NUM_47; // R2
            cfg.pin_d14 = GPIO_NUM_48; // R3
            cfg.pin_d15 = GPIO_NUM_45; // R4

            cfg.pin_henable = GPIO_NUM_41; // DE
            cfg.pin_vsync   = GPIO_NUM_40; // VSYNC
            cfg.pin_hsync   = GPIO_NUM_39; // HSYNC
            cfg.pin_pclk    = GPIO_NUM_0;  // PCLK
            cfg.freq_write  = 15000000;    // 15 MHz

            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 40;
            cfg.hsync_pulse_width = 48;
            cfg.hsync_back_porch  = 40;
            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 1;
            cfg.vsync_pulse_width = 31;
            cfg.vsync_back_porch  = 13;
            cfg.pclk_active_neg   = 1;
            cfg.de_idle_high      = 0;
            cfg.pclk_idle_high    = 0;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel configuration
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = SCREEN_WIDTH;
            cfg.memory_height = SCREEN_HEIGHT;
            cfg.panel_width   = SCREEN_WIDTH;
            cfg.panel_height  = SCREEN_HEIGHT;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }

        // Enable PSRAM for panel framebuffer
        {
            auto cfg = _panel_instance.config_detail();
            cfg.use_psram = 1;
            _panel_instance.config_detail(cfg);
        }

        setPanel(&_panel_instance);
    }
};

static LGFX tft;
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t* buf1 = nullptr;
static lv_disp_t* s_disp = nullptr;

static void lvgl_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.pushImage(area->x1, area->y1, w, h, (uint16_t*)&color_p->full);
    lv_disp_flush_ready(drv);
}

void display_init() {
    tft.begin();
    tft.setRotation(0);
    tft.setBrightness(255);

    lv_init();

    // Allocate draw buffer in PSRAM (~77KB for 1/10th screen)
    size_t buf_size = SCREEN_WIDTH * (SCREEN_HEIGHT / 10);
    buf1 = (lv_color_t*)ps_malloc(buf_size * sizeof(lv_color_t));
    if (!buf1) {
        // Fallback to regular malloc if PSRAM unavailable
        buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
    }
    lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = SCREEN_WIDTH;
    disp_drv.ver_res  = SCREEN_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    s_disp = lv_disp_drv_register(&disp_drv);

    // Set dark theme
    lv_theme_t* theme = lv_theme_default_init(
        s_disp,
        lv_color_hex(0x3B82F6),  // primary
        lv_color_hex(0xEF4444),  // secondary
        true,                     // dark mode
        LV_FONT_DEFAULT
    );
    lv_disp_set_theme(s_disp, theme);
}

lv_disp_t* display_get() {
    return s_disp;
}
