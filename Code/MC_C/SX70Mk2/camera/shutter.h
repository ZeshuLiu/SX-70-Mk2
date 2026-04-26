/**
 * 快门控制模块
 * 使用 RP2040 硬件定时器实现高精度曝光控制
 */

#ifndef SHUTTER_H
#define SHUTTER_H

#include <stdint.h>
#include <stdbool.h>

#define GPIO_DEBOUNCE_COUNT 5

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 去抖读取 GPIO，需连续 5 次高电平才判定为真
 */
bool gpio_debounce_defaultHigh(int pin);

/**
 * @brief 去抖读取 GPIO，默认低电平，需连续 5 次高电平才判定为 true
 */
bool gpio_debounce_defaultLow(int pin);

// 快门模式
#define SHUTTER_NORMAL      '1'     // 普通曝光模式
#define SHUTTER_FLASH       '0'     // 闪光灯模式
#define SHUTTER_BULB        'B'     // B 门模式
#define SHUTTER_TIME        'T'     // T 门模式

/**
 * @brief 初始化快门控制
 */
void shutter_init(void);

/**
 * @brief 关闭快门 (准备曝光)
 */
void shutter_close(void);

/**
 * @brief 开启快门 (开始曝光)
 */
void shutter_open(void);

/**
 * @brief 保持快门关闭状态
 */
void shutter_keep_closed(void);

/**
 * @brief 执行曝光控制
 * @param shutter_delay_ms 快门延时 (ms)
 * @param mode 快门模式 ('0'=闪光，'1'=正常，'B'=B 门，'T'=T 门)
 */
void shutter_expose(uint16_t shutter_delay_ms, char mode);

/**
 * @brief B 门模式处理 (按住全按快门保持开启)
 * @param s1t_pin 全按快门引脚
 * @return 实际曝光时间 (ms)
 */
uint16_t shutter_bulb_mode(uint8_t s1t_pin);

/**
 * @brief T 门模式处理 (按一次开启，再按一次关闭)
 * @param s1t_pin 全按快门引脚
 * @return 实际曝光时间 (ms)
 */
uint16_t shutter_time_mode(uint8_t s1t_pin);

/**
 * @brief 光圈就位 (闪光模式用)
 */
void aperture_engage(void);

/**
 * @brief 光圈归位
 */
void aperture_disengage(void);

#ifdef __cplusplus
}
#endif

#endif // SHUTTER_H
