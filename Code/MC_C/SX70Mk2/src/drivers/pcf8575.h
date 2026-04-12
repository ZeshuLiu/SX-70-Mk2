#ifndef PCF8575_H
#define PCF8575_H

#include <stdint.h>
#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// PCF8575 I2C 地址范围 (0x20 - 0x27)
// A0, A1, A2 引脚配置决定地址
#define PCF8575_I2C_ADDR_BASE   0x20

// PCF8575 配置结构体
typedef struct {
    i2c_inst_t *i2c_port;     // I2C 端口 (i2c0 或 i2c1)
    uint8_t i2c_addr;         // I2C 地址 (0x20-0x27)
    uint16_t pin_dir_mask;    // 方向掩码：1=输入，0=输出
    uint16_t pin_state;       // 当前输出状态
} pcf8575_t;

/**
 * @brief 初始化 PCF8575
 * @param dev PCF8575 设备指针
 * @param i2c_port I2C 端口 (i2c0 或 i2c1)
 * @param i2c_addr I2C 地址 (0x20-0x27)
 * @param pin_dir_mask 方向掩码 (1=输入，0=输出)
 * @return 0=成功，非 0=失败
 */
int pcf8575_init(pcf8575_t *dev, i2c_inst_t *i2c_port, uint8_t i2c_addr, uint16_t pin_dir_mask);

/**
 * @brief 读取 16 位 GPIO 输入状态
 * @param dev PCF8575 设备指针
 * @return 16 位 GPIO 状态 (高 8 位=P0，低 8 位=P1)
 */
uint16_t pcf8575_read(pcf8575_t *dev);

/**
 * @brief 写入 16 位 GPIO 输出状态
 * @param dev PCF8575 设备指针
 * @param value 16 位输出值 (高 8 位=P0，低 8 位=P1)
 */
void pcf8575_write(pcf8575_t *dev, uint16_t value);

/**
 * @brief 设置单个引脚为输出模式
 * @param dev PCF8575 设备指针
 * @param pin 引脚号 (0-15)
 */
void pcf8575_set_output(pcf8575_t *dev, uint8_t pin);

/**
 * @brief 设置单个引脚为输入模式
 * @param dev PCF8575 设备指针
 * @param pin 引脚号 (0-15)
 */
void pcf8575_set_input(pcf8575_t *dev, uint8_t pin);

/**
 * @brief 设置单个引脚输出电平
 * @param dev PCF8575 设备指针
 * @param pin 引脚号 (0-15)
 * @param value 电平值 (0=低，1=高)
 */
void pcf8575_write_pin(pcf8575_t *dev, uint8_t pin, uint8_t value);

/**
 * @brief 读取单个引脚输入电平
 * @param dev PCF8575 设备指针
 * @param pin 引脚号 (0-15)
 * @return 电平值 (0=低，1=高)
 */
uint8_t pcf8575_read_pin(pcf8575_t *dev, uint8_t pin);

/**
 * @brief 获取当前输出状态
 * @param dev PCF8575 设备指针
 * @return 当前输出状态值
 */
uint16_t pcf8575_get_state(pcf8575_t *dev);

#ifdef __cplusplus
}
#endif

#endif // PCF8575_H
