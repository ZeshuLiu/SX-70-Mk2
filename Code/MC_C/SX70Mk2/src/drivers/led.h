#ifndef LED_H
#define LED_H

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// LED 引脚定义
#define LED_Y_PIN    12    // 黄灯
#define LED_B_PIN    13    // 蓝灯

// LED 状态
#define LED_OFF      1     // 高电平关闭
#define LED_ON       0     // 低电平点亮

// LED 控制结构体
typedef struct {
    uint8_t led_y_state;   // 黄灯状态
    uint8_t led_b_state;   // 蓝灯状态
} led_state_t;

/**
 * @brief 初始化 LED
 */
void led_init(void);

/**
 * @brief 关闭所有 LED
 */
void led_close(void);

/**
 * @brief 根据 ISO 设置 LED
 * @param iso_600 ISO 600 时点亮黄灯，否则点亮蓝灯
 */
void led_iso(uint8_t iso_600);

/**
 * @brief 设置黄灯状态
 * @param state LED_ON 或 LED_OFF
 */
void led_y_set(uint8_t state);

/**
 * @brief 设置蓝灯状态
 * @param state LED_ON 或 LED_OFF
 */
void led_b_set(uint8_t state);

/**
 * @brief 获取 LED 状态
 * @return led_state_t 结构体
 */
led_state_t led_get_state(void);

#ifdef __cplusplus
}
#endif

#endif // LED_H
