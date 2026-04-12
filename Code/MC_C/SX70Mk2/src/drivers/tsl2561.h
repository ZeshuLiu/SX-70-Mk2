/**
 * TSL2561 测光传感器驱动
 * 基于 MicroPython 版本移植
 */

#ifndef TSL2561_H
#define TSL2561_H

#include <stdint.h>
#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// TSL2561 默认 I2C 地址 (0x39 是标准地址，但有些模块使用 0x29)
// Python 代码中使用地址 41 (0x29)
#define TSL2561_I2C_ADDR        0x29

// 命令寄存器位定义
#define TSL2561_COMMAND_BIT     0x80
#define TSL2561_WORD_BIT        0x20

// 寄存器地址
#define TSL2561_REG_CONTROL     0x00
#define TSL2561_REG_TIMING      0x01
#define TSL2561_REG_CHAN0_LOW   0x0C
#define TSL2561_REG_CHAN0_HIGH  0x0D
#define TSL2561_REG_CHAN1_LOW   0x0E
#define TSL2561_REG_CHAN1_HIGH  0x0F
#define TSL2561_REG_ID          0x0A

// 控制命令
#define TSL2561_CTRL_POWERON    0x03
#define TSL2561_CTRL_POWEROFF   0x00

// 增益定义
#define TSL2561_GAIN_1X         0
#define TSL2561_GAIN_16X        1

// 积分时间定义 (ms)
#define TSL2561_TIME_13MS       0
#define TSL2561_TIME_101MS      1
#define TSL2561_TIME_402MS      2
#define TSL2561_TIME_MANUAL     3

// LUX 计算参数
typedef struct {
    uint16_t k;
    uint16_t b;
    uint16_t m;
} lux_scale_t;

// TSL2561 设备结构体
typedef struct {
    i2c_inst_t *i2c_port;
    uint8_t i2c_addr;
    uint8_t gain;           // 0=1x, 1=16x
    uint8_t integration_time; // 0=13ms, 1=101ms, 2=402ms, 3=manual
    uint8_t active;
    uint16_t clip;
    uint32_t scale;
} tsl2561_t;

/**
 * @brief 初始化 TSL2561
 * @param dev 设备指针
 * @param i2c_port I2C 端口
 * @param i2c_addr I2C 地址 (默认 0x39)
 * @return 0=成功，非 0=失败
 */
int tsl2561_init(tsl2561_t *dev, i2c_inst_t *i2c_port, uint8_t i2c_addr);

/**
 * @brief 读取传感器 ID
 * @param dev 设备指针
 * @return 传感器 ID
 */
uint8_t tsl2561_read_id(tsl2561_t *dev);

/**
 * @brief 设置增益
 * @param dev 设备指针
 * @param gain 增益：TSL2561_GAIN_1X 或 TSL2561_GAIN_16X
 */
void tsl2561_set_gain(tsl2561_t *dev, uint8_t gain);

/**
 * @brief 读取通道数据
 * @param dev 设备指针
 * @param broadband 宽带通道值 (CH0)
 * @param ir 红外通道值 (CH1)
 * @return 0=成功，非 0=失败
 */
int tsl2561_read_channels(tsl2561_t *dev, uint16_t *broadband, uint16_t *ir);

/**
 * @brief 计算 LUX 值
 * @param dev 设备指针
 * @param broadband 宽带通道值
 * @param ir 红外通道值
 * @return LUX 值 (浮点数)
 */
float tsl2561_calc_lux(tsl2561_t *dev, uint16_t broadband, uint16_t ir);

/**
 * @brief 读取 LUX 值 (自动增益)
 * @param dev 设备指针
 * @return LUX 值 (浮点数)
 */
float tsl2561_read_lux(tsl2561_t *dev);

#ifdef __cplusplus
}
#endif

#endif // TSL2561_H
