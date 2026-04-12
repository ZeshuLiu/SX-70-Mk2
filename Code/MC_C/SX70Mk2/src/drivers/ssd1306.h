#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// SSD1306 默认 I2C 地址
#define SSD1306_I2C_ADDR        0x3C

// 屏幕尺寸
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          32
#define SSD1306_BUFFER_SIZE     (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

// SSD1306 命令
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_NORMAL_DISPLAY      0xA6
#define SSD1306_CMD_INVERT_DISPLAY      0xA7
#define SSD1306_CMD_SET_MUX_RATIO       0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_SET_SEGMENT_REMAP   0xA1
#define SSD1306_CMD_SET_COM_SCAN_DEC    0xC8
#define SSD1306_CMD_SET_COM_PINS        0xDA
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_VCOMH           0xDB
#define SSD1306_CMD_DISPLAY_ALL_ON_RES  0xA4
#define SSD1306_CMD_SET_CLK_DIV         0xD5
#define SSD1306_CMD_CHARGE_PUMP         0x8D
#define SSD1306_CMD_MEMORY_MODE         0x20
#define SSD1306_CMD_COLUMN_ADDR         0x21
#define SSD1306_CMD_PAGE_ADDR           0x22

// SSD1306 控制结构体
typedef struct {
    i2c_inst_t *i2c_port;
    uint8_t i2c_addr;
    uint8_t buffer[SSD1306_BUFFER_SIZE];
    uint8_t contrast;
} ssd1306_t;

/**
 * @brief 初始化 SSD1306
 * @param dev SSD1306 设备指针
 * @param i2c_port I2C 端口
 * @param i2c_addr I2C 地址
 * @return 0=成功，非 0=失败
 */
int ssd1306_init(ssd1306_t *dev, i2c_inst_t *i2c_port, uint8_t i2c_addr);

/**
 * @brief 打开显示
 * @param dev SSD1306 设备指针
 */
void ssd1306_display_on(ssd1306_t *dev);

/**
 * @brief 关闭显示
 * @param dev SSD1306 设备指针
 */
void ssd1306_display_off(ssd1306_t *dev);

/**
 * @brief 设置对比度
 * @param dev SSD1306 设备指针
 * @param contrast 对比度值 (0-255)
 */
void ssd1306_set_contrast(ssd1306_t *dev, uint8_t contrast);

/**
 * @brief 清空缓冲区
 * @param dev SSD1306 设备指针
 */
void ssd1306_clear(ssd1306_t *dev);

/**
 * @brief 刷新显示
 * @param dev SSD1306 设备指针
 */
void ssd1306_show(ssd1306_t *dev);

/**
 * @brief 设置像素点
 * @param dev SSD1306 设备指针
 * @param x X 坐标 (0-127)
 * @param y Y 坐标 (0-31)
 * @param color 颜色 (0=黑，1=白)
 */
void ssd1306_set_pixel(ssd1306_t *dev, int16_t x, int16_t y, uint8_t color);

/**
 * @brief 画线
 * @param dev SSD1306 设备指针
 * @param x0 起点 X
 * @param y0 起点 Y
 * @param x1 终点 X
 * @param y1 终点 Y
 * @param color 颜色
 */
void ssd1306_draw_line(ssd1306_t *dev, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);

/**
 * @brief 画矩形
 * @param dev SSD1306 设备指针
 * @param x 左上角 X
 * @param y 左上角 Y
 * @param w 宽度
 * @param h 高度
 * @param color 颜色 (0=黑，1=白)，使用填充模式
 */
void ssd1306_fill_rect(ssd1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);

/**
 * @brief 清空/填充整个屏幕
 * @param dev SSD1306 设备指针
 * @param color 颜色 (0=黑，1=白)
 */
void ssd1306_fill(ssd1306_t *dev, uint8_t color);

/**
 * @brief 绘制字符 (5x7 字体)
 * @param dev SSD1306 设备指针
 * @param x X 坐标
 * @param y Y 坐标
 * @param ch 字符
 * @param color 颜色
 */
void ssd1306_draw_char(ssd1306_t *dev, int16_t x, int16_t y, char ch, uint8_t color);

/**
 * @brief 绘制字符串
 * @param dev SSD1306 设备指针
 * @param x X 坐标
 * @param y Y 坐标
 * @param str 字符串
 * @param color 颜色
 */
void ssd1306_draw_string(ssd1306_t *dev, int16_t x, int16_t y, const char *str, uint8_t color);

#ifdef __cplusplus
}
#endif

#endif // SSD1306_H
