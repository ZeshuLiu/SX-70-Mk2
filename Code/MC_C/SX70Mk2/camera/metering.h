/**
 * 测光模块 - 曝光计算
 * 基于 TSL2561 传感器数据计算快门速度
 */

#ifndef METERING_H
#define METERING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 快门速度表 (外部可访问)
extern const char *shutter_speeds[];
#define SHUTTER_SPEED_COUNT 22

// 快门速度时间定义 (0.1ms) - 来自 Python camera_driver.py 1.2.4 版本
// 原 ms 值×10，用于高精度快门定时器
extern const uint16_t shutter_times_x10[];

/**
 * @brief 根据 LUX 值计算快门速度 (ISO 600)
 * @param lux LUX 值 (已经过 (lux-0.4)*0.9 调整)
 * @return 快门速度在 shutter_speeds 数组中的索引 (1-19 对应 ev7-ev17)
 */
uint8_t calc_shutter_from_lux(float lux);

/**
 * @brief 获取快门速度字符串
 * @param index 索引值
 * @return 快门速度字符串 (如 "1/2", "1/125" 等)
 */
const char* get_shutter_speed(uint8_t index);

/**
 * @brief 获取快门速度时间 (0.1ms)
 * @param index 索引值
 * @return 快门时间 (0.1ms 单位)
 */
uint16_t get_shutter_time_x10(uint8_t index);

/**
 * @brief 执行测光
 * @param last_lux 输出参数：存储上次测光值
 * @param auto_shutter_pos 输出参数：存储计算的快门位置
 * @return 调整后的 LUX 值
 */
float do_meter(float *last_lux, uint8_t *auto_shutter_pos);

/**
 * @brief 执行测光
 * @param last_lux 输出参数：存储上次测光值
 * @param auto_shutter_pos 输出参数：存储计算的快门位置
 * @return 调整后的 LUX 值
 */
float do_meter(float *last_lux, uint8_t *auto_shutter_pos);

#ifdef __cplusplus
}
#endif

#endif // METERING_H
