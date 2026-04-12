/**
 * TSL2561 测光传感器驱动
 * 基于 MicroPython 版本移植
 */

#include "tsl2561.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

// 积分时间参数表
// time_reg, wait_ms, clip, min_val, max_val, scale
static const struct {
    uint8_t time_reg;
    uint16_t wait_ms;
    uint16_t clip;
    uint16_t min_val;
    uint16_t max_val;
    uint32_t scale;
} integration_times[] = {
    { 0x00, 15,  4900,  100,  4850, 0x7517 },  // 13ms
    { 0x01, 120, 37000, 200, 36000, 0x0FE7 },  // 101ms
    { 0x02, 450, 65000, 500, 63000, 0x0400 },  // 402ms (scale = 1 << 10 = 1024)
    { 0x03, 0,   0,     0,   0,     0      },  // manual
};

// LUX 计算参数表
static const lux_scale_t lux_scale[] = {
    { 0x0040, 0x01f2, 0x01be },
    { 0x0080, 0x0214, 0x02d1 },
    { 0x00c0, 0x023f, 0x037b },
    { 0x0100, 0x0270, 0x03fe },
    { 0x0138, 0x016f, 0x01fc },
    { 0x019a, 0x00d2, 0x00fb },
    { 0x029a, 0x0018, 0x0012 },
};

static uint8_t read_reg8(tsl2561_t *dev, uint8_t reg) {
    uint8_t buf[1];
    reg |= TSL2561_COMMAND_BIT;
    i2c_write_blocking(dev->i2c_port, dev->i2c_addr, &reg, 1, true);
    i2c_read_blocking(dev->i2c_port, dev->i2c_addr, buf, 1, false);
    return buf[0];
}

static void write_reg8(tsl2561_t *dev, uint8_t reg, uint8_t value) {
    uint8_t buf[2];
    reg |= TSL2561_COMMAND_BIT;
    buf[0] = reg;
    buf[1] = value;
    i2c_write_blocking(dev->i2c_port, dev->i2c_addr, buf, 2, false);
}

static uint16_t read_reg16(tsl2561_t *dev, uint8_t reg) {
    uint8_t buf[2];
    reg |= TSL2561_COMMAND_BIT | TSL2561_WORD_BIT;
    i2c_write_blocking(dev->i2c_port, dev->i2c_addr, &reg, 1, true);
    i2c_read_blocking(dev->i2c_port, dev->i2c_addr, buf, 2, false);
    return (buf[1] << 8) | buf[0];
}

static void write_reg16(tsl2561_t *dev, uint8_t reg, uint16_t value) {
    uint8_t buf[3];
    reg |= TSL2561_COMMAND_BIT | TSL2561_WORD_BIT;
    buf[0] = reg;
    buf[1] = value & 0xFF;
    buf[2] = (value >> 8) & 0xFF;
    i2c_write_blocking(dev->i2c_port, dev->i2c_addr, buf, 3, false);
}

static void update_gain_and_time(tsl2561_t *dev) {
    uint8_t timing = integration_times[dev->integration_time].time_reg;
    timing |= (dev->gain == TSL2561_GAIN_16X) ? 0x10 : 0x00;
    write_reg8(dev, TSL2561_REG_TIMING, timing);
}

int tsl2561_init(tsl2561_t *dev, i2c_inst_t *i2c_port, uint8_t i2c_addr) {
    dev->i2c_port = i2c_port;
    dev->i2c_addr = i2c_addr;
    dev->gain = TSL2561_GAIN_1X;
    dev->integration_time = TSL2561_TIME_13MS;
    dev->active = 0;

    // 读取传感器 ID 验证
    uint8_t id = tsl2561_read_id(dev);
    if (!(id & 0x10)) {
        return -1;  // 传感器 ID 不正确
    }

    // 设置默认增益和积分时间
    update_gain_and_time(dev);
    dev->clip = integration_times[dev->integration_time].clip;
    dev->scale = integration_times[dev->integration_time].scale;

    return 0;
}

uint8_t tsl2561_read_id(tsl2561_t *dev) {
    return read_reg8(dev, TSL2561_REG_ID);
}

void tsl2561_set_gain(tsl2561_t *dev, uint8_t gain) {
    dev->gain = gain;
    update_gain_and_time(dev);
}

int tsl2561_read_channels(tsl2561_t *dev, uint16_t *broadband, uint16_t *ir) {
    // 上电
    write_reg8(dev, TSL2561_REG_CONTROL, TSL2561_CTRL_POWERON);

    // 等待积分时间
    sleep_ms(integration_times[dev->integration_time].wait_ms);

    // 读取通道数据
    *broadband = read_reg16(dev, TSL2561_REG_CHAN0_LOW);
    *ir = read_reg16(dev, TSL2561_REG_CHAN1_LOW);

    // 断电
    write_reg8(dev, TSL2561_REG_CONTROL, TSL2561_CTRL_POWEROFF);

    return 0;
}

float tsl2561_calc_lux(tsl2561_t *dev, uint16_t broadband, uint16_t ir) {
    // 检查饱和
    uint16_t clip = integration_times[dev->integration_time].clip;
    if (broadband > clip || ir > clip) {
        return -1.0f;  // 饱和错误
    }

    // Python: scale = _INTEGRATION_TIME[self._integration_time][5] / self._gain
    uint32_t scale = integration_times[dev->integration_time].scale / (dev->gain == TSL2561_GAIN_16X ? 16 : 1);

    // Python: channel0 = (broadband * scale) / 1024
    float channel0 = ((float)broadband * scale) / 1024.0f;
    float channel1 = ((float)ir * scale) / 1024.0f;

    // Python: ratio = (((channel1 * 1024) / channel0 if channel0 else 0) + 1) / 2
    float ratio = channel0 ? (((channel1 * 1024.0f) / channel0) + 1.0f) / 2.0f : 0.0f;

    // 查找合适的参数
    uint16_t b = 0, m = 0;
    for (int i = 0; i < 7; i++) {
        if (ratio <= lux_scale[i].k) {
            b = lux_scale[i].b;
            m = lux_scale[i].m;
            break;
        }
    }

    // Python: return (max(0, channel0 * b - channel1 * m) + 8192) / 16384
    float lux = channel0 * b - channel1 * m;
    if (lux < 0.0f) lux = 0.0f;
    lux = (lux + 8192.0f) / 16384.0f;

    return lux;
}

float tsl2561_read_lux(tsl2561_t *dev) {
    uint16_t broadband, ir;

    // Python read(autogain=True) 逻辑：
    // 1. 先用 16x 增益读取
    // 2. 如果值太小 (< min_val)，保持 16x
    // 3. 如果值太大 (> max_val)，切换到 1x 增益
    // 4. 返回 LUX 值

    // 先用 16x 增益读取
    tsl2561_set_gain(dev, TSL2561_GAIN_16X);
    tsl2561_read_channels(dev, &broadband, &ir);

    // 检查是否需要调整增益
    if (broadband > integration_times[dev->integration_time].max_val) {
        // 信号太强，切换到 1x 增益
        tsl2561_set_gain(dev, TSL2561_GAIN_1X);
        tsl2561_read_channels(dev, &broadband, &ir);
    }
    // 如果信号太弱，保持 16x 增益

    return tsl2561_calc_lux(dev, broadband, ir);
}
