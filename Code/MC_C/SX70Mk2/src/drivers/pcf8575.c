#include "pcf8575.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

// PCF8575 上电复位状态：所有引脚配置为输入，输出寄存器全为 1

int pcf8575_init(pcf8575_t *dev, i2c_inst_t *i2c_port, uint8_t i2c_addr, uint16_t pin_dir_mask) {
    if (!dev || i2c_addr < 0x20 || i2c_addr > 0x27) {
        return -1;
    }

    dev->i2c_port = i2c_port;
    dev->i2c_addr = i2c_addr;
    dev->pin_dir_mask = pin_dir_mask;
    dev->pin_state = pin_dir_mask & 0xFFFF;  // 初始化输出状态与方向一致

    // 写入初始状态
    uint8_t buffer[2];
    buffer[0] = dev->pin_state & 0xFF;          // P0 低 8 位
    buffer[1] = (dev->pin_state >> 8) & 0xFF;   // P1 高 8 位

    int ret = i2c_write_blocking(dev->i2c_port, dev->i2c_addr, buffer, 2, false);
    return (ret == 2) ? 0 : -1;
}

uint16_t pcf8575_read(pcf8575_t *dev) {
    uint8_t buffer[2];

    int ret = i2c_read_blocking(dev->i2c_port, dev->i2c_addr, buffer, 2, false);

    if (ret != 2) {
        return 0xFFFF;  // 读取失败返回全 1
    }

    // PCF8575 返回格式：低字节=P0, 高字节=P1
    return ((uint16_t)buffer[1] << 8) | buffer[0];
}

void pcf8575_write(pcf8575_t *dev, uint16_t value) {
    uint8_t buffer[2];

    // 只更新输出引脚的状态
    // 输入引脚保持为 1 (上拉)
    uint16_t output_value = (value & ~dev->pin_dir_mask) | (dev->pin_dir_mask & 0xFFFF);

    buffer[0] = output_value & 0xFF;           // P0 低 8 位
    buffer[1] = (output_value >> 8) & 0xFF;    // P1 高 8 位

    dev->pin_state = output_value;

    i2c_write_blocking(dev->i2c_port, dev->i2c_addr, buffer, 2, false);
}

void pcf8575_set_output(pcf8575_t *dev, uint8_t pin) {
    if (pin > 15) return;

    uint16_t mask = ~(1U << pin);
    dev->pin_dir_mask &= mask;

    // 重新写入当前状态
    pcf8575_write(dev, dev->pin_state);
}

void pcf8575_set_input(pcf8575_t *dev, uint8_t pin) {
    if (pin > 15) return;

    uint16_t mask = (1U << pin);
    dev->pin_dir_mask |= mask;

    // 输入引脚需要输出 1 (上拉)
    uint16_t new_state = dev->pin_state | mask;
    pcf8575_write(dev, new_state);
}

void pcf8575_write_pin(pcf8575_t *dev, uint8_t pin, uint8_t value) {
    if (pin > 15) return;

    // 检查是否为输出引脚
    if (dev->pin_dir_mask & (1U << pin)) {
        return;  // 输入引脚不能写入
    }

    if (value) {
        dev->pin_state |= (1U << pin);
    } else {
        dev->pin_state &= ~(1U << pin);
    }

    pcf8575_write(dev, dev->pin_state);
}

uint8_t pcf8575_read_pin(pcf8575_t *dev, uint8_t pin) {
    if (pin > 15) return 0;

    uint16_t state = pcf8575_read(dev);
    return (state & (1U << pin)) ? 1 : 0;
}

uint16_t pcf8575_get_state(pcf8575_t *dev) {
    return dev->pin_state;
}
