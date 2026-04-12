/**
 * 测光模块 - 曝光计算
 * 基于 TSL2561 传感器数据计算快门速度
 */

#include "metering.h"
#include "drivers/tsl2561.h"
#include "pico/stdlib.h"

// 外部测光表实例
extern tsl2561_t lm;

// 快门速度表 (Python 程序中的 M_CMD_Dict)
const char *shutter_speeds[] = {
    "1s", "1/2", "1/3", "1/4", "1/6", "1/8", "1/10", "1/15", "1/20",
    "1/30", "1/45", "1/60", "1/90", "1/125", "1/180", "1/250", "1/360",
    "1/500", "1/1000", "1/2000"
};
#define SHUTTER_SPEED_COUNT 20

// 快门速度时间定义 (ms) - 来自 Python camera_driver.py 1.2.4 版本
// 用于高精度快门定时器
const uint16_t shutter_times_ms[] = {
    1050,  // ev6  = 1s
    540,   // ev7  = 1/2
    470,   // ev75 = 1/3
    300,   // ev8  = 1/4
    290,   // ev85 = 1/6
    175,   // ev9  = 1/8
    136,   // ev95 = 1/10
    97,    // ev10 = 1/15
    79,    // ev105= 1/20
    60,    // ev11 = 1/30
    52,    // ev115= 1/45
    45,    // ev12 = 1/60
    41,    // ev125= 1/90
    37,    // ev13 = 1/125
    34,    // ev135= 1/180
    32,    // ev14 = 1/250
    28,    // ev145= 1/360
    25,    // ev15 = 1/500
    23,    // ev16 = 1/1000
    22     // ev17 = 1/2000
};

uint8_t calc_shutter_from_lux(float lux) {
    // Python 中的阈值判断 (完全对应)
    // lux <= 0.120 返回 ev7 (1/2s) - 对应索引 1
    if (lux <= 0.120f) return 1;   // ev7 (1/2s)
    if (lux <= 0.125f) return 2;   // ev75 (1/3s)
    if (lux <= 0.1385f) return 3;  // ev8 (1/4s)
    if (lux <= 0.152f) return 4;   // ev85 (1/6s)
    if (lux <= 0.185f) return 5;   // ev9 (1/8s)
    if (lux <= 0.22f) return 6;    // ev95 (1/10s)
    if (lux <= 0.275f) return 7;   // ev10 (1/15s)
    if (lux <= 0.345f) return 8;   // ev105 (1/20s)
    if (lux <= 0.468f) return 9;   // ev11 (1/30s)
    if (lux <= 0.58f) return 10;   // ev115 (1/45s)
    if (lux <= 0.802f) return 11;  // ev12 (1/60s)
    if (lux <= 1.115f) return 12;  // ev125 (1/90s)
    if (lux <= 1.6f) return 13;    // ev13 (1/125s)
    if (lux <= 2.35f) return 14;   // ev135 (1/180s)
    if (lux <= 3.4f) return 15;    // ev14 (1/250s)
    if (lux <= 4.2f) return 16;    // ev145 (1/360s)
    if (lux <= 10.0f) return 17;   // ev15 (1/500s)
    if (lux <= 100.0f) return 18;  // ev16 (1/1000s)
    return 19; // ev17 (1/2000s)
}

// 快门速度表访问函数
const char* get_shutter_speed(uint8_t index) {
    if (index >= SHUTTER_SPEED_COUNT) {
        return shutter_speeds[SHUTTER_SPEED_COUNT - 1];
    }
    return shutter_speeds[index];
}

// 获取快门速度时间 (ms)
uint16_t get_shutter_time_ms(uint8_t index) {
    if (index >= SHUTTER_SPEED_COUNT) {
        return shutter_times_ms[SHUTTER_SPEED_COUNT - 1];
    }
    return shutter_times_ms[index];
}

float do_meter(float *last_lux, uint8_t *auto_shutter_pos) {
    float lux_sum = 0.0f;
    int valid_count = 0;

    // Python: 连续读取 7 次取平均
    // tsl2561_read_lux() 内部已经处理了自动增益切换
    for (int i = 0; i < 7; i++) {
        float lux = tsl2561_read_lux(&lm);
        // 检查是否饱和（返回 -1.0f 表示饱和）
        if (lux >= 0.0f) {
            lux_sum += lux;
            valid_count++;
        }
        if (i < 6) sleep_ms(1);
    }

    if (valid_count == 0) {
        if (last_lux) *last_lux = 0.0f;
        if (auto_shutter_pos) *auto_shutter_pos = 1;  // ev7 (1/2s) 最暗
        return 0.0f;
    }

    float lux_avg = lux_sum / valid_count;

    // Python: lux -= 0.4; lux *= 0.9
    float lux_adj = (lux_avg - 0.4f) * 0.9f;
    if (lux_adj < 0.0f) lux_adj = 0.0f;

    if (last_lux) *last_lux = lux_adj;

    // 计算 AUTO 档的快门速度
    uint8_t shutter_pos = calc_shutter_from_lux(lux_adj);
    if (shutter_pos >= SHUTTER_SPEED_COUNT) {
        shutter_pos = SHUTTER_SPEED_COUNT - 1;
    }
    if (auto_shutter_pos) *auto_shutter_pos = shutter_pos;

    return lux_adj;
}
