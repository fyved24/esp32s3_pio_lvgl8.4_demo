/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <demos/lv_demos.h>

/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 *You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
 as the examples and demos are now part of the main LVGL library. */

/*Change to your screen resolution*/
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

#include "FT6336U.h"
// FT6336U pins
#define FT6336U_INT 26 // T_IRQ
#define FT6336U_SDA 21 // T_SDI
#define FT6336U_RST 27 // T_CS
#define FT6336U_SCL 22 // T_CLK

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char *buf)
{
    LV_UNUSED(level);
    Serial.println(buf);
    Serial.flush();
}

FT6336U ft6336u(FT6336U_SDA, FT6336U_SCL, FT6336U_RST, FT6336U_INT);
// Get the Touchscreen data

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp_drv);
}

void my_touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    static lv_indev_state_t last_state = LV_INDEV_STATE_REL; // 记录上一次的触摸状态
    FT6336U_TouchPointType touchPoint = ft6336u.scan();  // 检查触摸屏是否有触摸数据
    if (touchPoint.touch_count > 0) {  
        data->state = LV_INDEV_STATE_PR;  // 设置触摸状态为按下
        data->point.y = touchPoint.tp[0].x;  // 获取触摸点的Y坐标
        data->point.x = 319 - touchPoint.tp[0].y;  // 获取触摸点的X坐标
        if (last_state != LV_INDEV_STATE_PR) {  // 如果上次状态不是按下
            Serial.println("Touched X = " + String(data->point.x) + " Y = " + String(data->point.y));  // 在串口打印触摸坐标
            Serial.print("FT6336U TD Status: ");
            Serial.println(ft6336u.read_td_status());
            Serial.print("FT6336U Touch Event/ID 1: (");
            Serial.print(ft6336u.read_touch1_event()); Serial.print(" / "); Serial.print(ft6336u.read_touch1_id()); Serial.println(")");
            Serial.print("FT6336U Touch Position 1: (");
            Serial.print(ft6336u.read_touch1_x()); Serial.print(" , "); Serial.print(ft6336u.read_touch1_y()); Serial.println(")");
            Serial.print("FT6336U Touch Weight/MISC 1: (");
            Serial.print(ft6336u.read_touch1_weight()); Serial.print(" / "); Serial.print(ft6336u.read_touch1_misc()); Serial.println(")");
            Serial.print("FT6336U Touch Event/ID 2: (");
            Serial.print(ft6336u.read_touch2_event()); Serial.print(" / "); Serial.print(ft6336u.read_touch2_id()); Serial.println(")");
            Serial.print("FT6336U Touch Position 2: (");
            Serial.print(ft6336u.read_touch2_x()); Serial.print(" , "); Serial.print(ft6336u.read_touch2_y()); Serial.println(")");
            Serial.print("FT6336U Touch Weight/MISC 2: (");
            Serial.print(ft6336u.read_touch2_weight()); Serial.print(" / "); Serial.print(ft6336u.read_touch2_misc()); Serial.println(")");
            last_state = LV_INDEV_STATE_PR;  // 更新触摸状态
        }
    } else {
        if (last_state != LV_INDEV_STATE_REL) {  // 如果上次状态不是释放
            Serial.println("Touch released");  // 在串口打印触摸释放信息
            last_state = LV_INDEV_STATE_REL;  // 更新触摸状态为释放
        }
        data->state = LV_INDEV_STATE_REL;  // 设置触摸状态为释放
    }
}
void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println(LVGL_Arduino);
    Serial.println("I am LVGL_Arduino");

    lv_init();
    ft6336u.begin();

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

    tft.begin();        /* TFT init */
    tft.setRotation(3); /* Landscape orientation, flipped */

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    /* Create simple label */
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello Ardino and LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    /* Try an example. See all the examples
     * online: https://docs.lvgl.io/master/examples.html
     * source codes: https://github.com/lvgl/lvgl/tree/e7f88efa5853128bf871dde335c0ca8da9eb7731/examples */
    // lv_example_btn_1();

    /*Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMOS_WIDGETS*/
    lv_demo_widgets();
    //  lv_demo_benchmark();
    //  lv_demo_keypad_encoder();
    //  lv_demo_music();
    //  lv_demo_printer();
    // lv_demo_stress();

    Serial.println("Setup done");
}

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}