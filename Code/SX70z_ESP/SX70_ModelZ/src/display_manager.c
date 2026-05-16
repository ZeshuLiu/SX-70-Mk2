#include "display_manager.h"
#include "fonts/font5x8.h"
#include "PIN.h"
#include "driver/gpio.h"
#include <stdio.h>

void display_show_frame(const camera_state_t *state, ssd1306_t *disp)
{
    if (!state->if_display) return;

    ssd1306_clear(disp);

    // 顶部状态栏 — 反色（白底黑字）
    ssd1306_fill_rect(disp, 0, 0, 128, 11);

    // 模式显示区
    ssd1306_clear_rect(disp, 1, 1, 48, 9);
    ssd1306_draw_str(disp, 10, 2, state->cam_mode, &font5x8_font);

    // ISO 显示
    ssd1306_clear_rect(disp, 52, 1, 28, 9);
    ssd1306_clear_rect(disp, 51, 0, 1, 11);   // 分隔线
    ssd1306_draw_str(disp, 55, 2, "600", &font5x8_font);

    // 闪光灯指示
    ssd1306_clear_rect(disp, 82, 0, 1, 11);   // 分隔线
    bool flash_connected = (gpio_get_level(S2_PIN) == 0);
    if (flash_connected) {
        ssd1306_draw_str(disp, 86, 2, "FLASH", &font5x8_font);
    } else {
        ssd1306_draw_str(disp, 90, 2, "OFF", &font5x8_font);
    }

    // 快门速度大字
    ssd1306_draw_str(disp, 8, 18,
                    get_shutter_speed(state->shutter_speed),
                    &font5x8_font);

    // 测光值（右下角）
    char lux_str[16];
    if (state->menu == 0) {
        snprintf(lux_str, sizeof(lux_str), "L:%.2f", state->metering.last_lux);
    } else {
        snprintf(lux_str, sizeof(lux_str), "L:---");
    }
    ssd1306_draw_str(disp, 75, 20, lux_str, &font5x8_font);

    ssd1306_show(disp);
}
