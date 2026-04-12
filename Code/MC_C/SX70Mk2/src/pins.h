#ifndef PINS_H
#define PINS_H

// ==================== SX-70 Mk2 引脚定义 ====================

// I2C 端口定义
#define I2C_PORT_LM         i2c0        // 测光表 I2C
#define I2C_PIN_LM_SDA      20
#define I2C_PIN_LM_SCL      21

#define I2C_PORT_PLUG       i2c1        // 外接控制器 I2C (PCF8575)
#define I2C_PIN_PLUG_SDA    18
#define I2C_PIN_PLUG_SCL    19

// 直接连接的按键引脚 (Pico GPIO)
#define S1F_PIN             2           // Sw 1 Focus (半按对焦)
#define S1T_PIN             1           // Sw 1 Take Photo (全按拍摄)
#define S2_PIN              14          // Flash Check (闪光检测)
#define S3_PIN              7           // 反光板位置检测
#define S5_PIN              6           // 胶片检测

// 输出控制引脚
#define SHUTTER_PIN         9           // 快门 PWM
#define APERTURE_PIN        17          // 光圈 PWM
#define MOTOR_PIN           5           // 马达控制
#define LED_Y_PIN           12          // 黄灯
#define LED_B_PIN           13          // 蓝灯
#define FF_PIN              11          // 闪光灯触发
#define S1F_FBW_PIN         22          // S1F 反馈

// ADC 引脚 (测光)
#define ADC_STAGE1_PIN      26          // ADC0 - 测光通道 1
#define ADC_STAGE2_PIN      27          // ADC1 - 测光通道 2

// PCF8575 配置 (外接 I2C 扩展)
#define PCF8575_I2C_PORT    I2C_PORT_PLUG
#define PCF8575_I2C_ADDR    0x20        // I2C 地址 (32 十进制)

// PCF8575 引脚定义 (16 位 GPIO)
// 注意：Python PCF8575 库使用 0-7 和 10-17 编号，C 代码使用标准 0-15 bit 位置
// 转换关系：Python 10-17 → C 8-15 (减 2)
// 3D 按键：连接在 P12, P10, P11 (Python 编号) → bit 10, 8, 9 (C 编号)
#define PCF_BUTTON3D_DOWN   10          // 3D 按键 - 下 (Python P12 → C bit 10)
#define PCF_BUTTON3D_UP     8           // 3D 按键 - 上 (Python P10 → C bit 8)
#define PCF_BUTTON3D_PUSH   9           // 3D 按键 - 按下 (Python P11 → C bit 9)

// 编码器（预留）
#define PCF_ENC_A           4           // 主编码器 A 相
#define PCF_ENC_B           5           // 主编码器 B 相
#define PCF_P3              3
#define PCF_P4              4
#define PCF_P5              5
#define PCF_P6              6
#define PCF_P7              7
#define PCF_P8              8
#define PCF_P9              9
#define PCF_P10             10
#define PCF_P11             11
#define PCF_P12             12
#define PCF_P13             13
#define PCF_P14             14
#define PCF_P15             15

// 编码器引脚掩码
#define ENC_PIN_A           PCF_P0
#define ENC_PIN_B           PCF_P1

#endif // PINS_H
