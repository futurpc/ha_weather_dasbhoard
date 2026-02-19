#include "touch.h"
#include "config.h"
#include <Wire.h>
#include <TAMC_GT911.h>

#define TOUCH_SDA 19
#define TOUCH_SCL 20
#define TOUCH_INT -1
#define TOUCH_RST -1

// Coordinate mapping (from Elecrow reference)
#define TOUCH_MAP_X1 800
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 480
#define TOUCH_MAP_Y2 0

static TAMC_GT911 tp(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST,
                     max(TOUCH_MAP_X1, TOUCH_MAP_X2),
                     max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));
static lv_indev_drv_t indev_drv;
static int16_t touch_last_x = 0;
static int16_t touch_last_y = 0;

static bool touch_touched() {
    tp.read();
    if (tp.isTouched) {
        touch_last_x = map(tp.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, SCREEN_WIDTH - 1);
        touch_last_y = map(tp.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, SCREEN_HEIGHT - 1);
        return true;
    }
    return false;
}

static void touch_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    if (touch_touched()) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void touch_init(lv_disp_t* disp) {
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    tp.begin();
    tp.setRotation(ROTATION_NORMAL);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    indev_drv.disp    = disp;
    lv_indev_drv_register(&indev_drv);
}
