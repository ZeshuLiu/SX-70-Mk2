/**
 * 快门控制模块
 * 使用 RP2040 硬件定时器实现高精度曝光控制
 */

#include "shutter.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "pins.h"
#include <stdio.h>

// 快门 PWM 切片号
static uint shutter_pwm_slice, shutter_channel_num;

// 光圈 PWM 切片号
static uint aperture_pwm_slice, aperture_channel_num;

// 定时器回调标识
static volatile bool shutter_timer_fired = false;

// 快门关闭定时器回调
static int64_t shutter_close_callback(alarm_id_t id, void *user_data) {
    shutter_timer_fired = true;
    shutter_close();
    return 0;
}

void shutter_init(void) {
    // 初始化 GPIO 为 PWM 功能
    gpio_set_function(SHUTTER_PIN, GPIO_FUNC_PWM);

    // 获取 PWM 切片号
    shutter_pwm_slice = pwm_gpio_to_slice_num(SHUTTER_PIN);
    shutter_channel_num = pwm_gpio_to_channel(SHUTTER_PIN);

    // 设置 PWM 频率 (与 Python 一致)
    // 公式：PWM 频率 = 系统时钟 / (分频值 × wrap)
    // 系统时钟 = 125MHz
    //
    // 最终配置:
    //   wrap = 999 (1000 步分辨率)
    //   div = 4.0
    //   频率 = 125MHz / (4 × 1000) = 31.25 kHz
    //
    // Python: shutter.freq(20000)
    pwm_set_wrap(shutter_pwm_slice, 999);        // 1000步
    // 设置初始占空比（0%）
    pwm_set_chan_level(shutter_pwm_slice, shutter_channel_num, 0);
    // pwm_set_clkdiv_int_frac(shutter_pwm_slice, 95, 0);
    // pwm_set_wrap(shutter_pwm_slice, 65535);  // 16 位分辨率

    pwm_set_clkdiv(shutter_pwm_slice, 4.0f);

    // 使能 PWM
    pwm_set_enabled(shutter_pwm_slice, true);

    // 初始化光圈 PWM (70kHz)
    // Python: aperture.freq(70000)
    gpio_set_function(APERTURE_PIN, GPIO_FUNC_PWM);
    aperture_pwm_slice = pwm_gpio_to_slice_num(APERTURE_PIN);
    aperture_channel_num = pwm_gpio_to_channel(APERTURE_PIN);

    // 70kHz: wrap=999, div = 125MHz / (70kHz × 1000) ≈ 1.786
    pwm_set_wrap(aperture_pwm_slice, 999);
    pwm_set_clkdiv(aperture_pwm_slice, 1.786f);
    pwm_set_chan_level(aperture_pwm_slice, aperture_channel_num, 0);  // 初始关闭
    pwm_set_enabled(aperture_pwm_slice, true);
}

void shutter_close(void) {
    // 关闭快门 (对应 Python duty_u16(65535))
    pwm_set_chan_level(shutter_pwm_slice, shutter_channel_num, 1000);
}

void shutter_open(void) {
    // 开启快门 (对应 Python duty_u16(0))
    pwm_set_chan_level(shutter_pwm_slice, shutter_channel_num, 0);
}

void shutter_keep_closed(void) {
    // 保持快门关闭 (对应 Python duty_u16(30000))
    pwm_set_chan_level(shutter_pwm_slice, shutter_channel_num, 1000);
}

void aperture_engage(void) {
    // 光圈就位 (对应 Python duty_u16(65535))
    pwm_set_chan_level(aperture_pwm_slice, aperture_channel_num, 1000);
}

void aperture_disengage(void) {
    // 光圈归位 (对应 Python duty_u16(0))
    pwm_set_chan_level(aperture_pwm_slice, aperture_channel_num, 0);
}

void shutter_expose(uint16_t shutter_delay_x10, char mode) {
    // 拍摄前准备
    printf("Taking Picture\r\n");
    printf("Shutter start to close!\r\n");

    // 关闭快门
    shutter_close();
    sleep_ms(30);
    shutter_keep_closed();

    printf("Shutter closed!\r\n");
    sleep_ms(30);  // 等快门完全关闭

    // 电机启动，带动反光板上升
    gpio_put(MOTOR_PIN, 1);
    printf("Motor Start Moving\r\n");

    // 等待反光板就位 (S3 引脚检测)
    while (gpio_get(S3_PIN) == 0) {
        tight_loop_contents();
    }

    gpio_put(MOTOR_PIN, 0);
    printf("Motor Stoped!\r\n");

    // Y Delay (光圈就位 + 自拍延时)
    if (mode == SHUTTER_FLASH) {
        // 闪光模式：光圈就位
        aperture_engage();
        printf("Aperture engaged (Flash mode)\r\n");
    }
    sleep_ms(18);  // 基础 Y delay

    // 开启快门，曝光开始
    printf("Shutter Start to Open, Exposure Starts\r\n");

    if (mode == SHUTTER_NORMAL) {
        // 普通曝光模式 - 使用硬件定时器
        // shutter_delay_x10 单位是 0.1ms，转换成 us 需要×100
        printf("Normal Mode! delay=%d (0.1ms)\r\n", shutter_delay_x10);

        shutter_timer_fired = false;
        shutter_open();

        // 启动定时器 (单位 us)
        int64_t alarm_id = add_alarm_in_us(shutter_delay_x10 * 100, shutter_close_callback, NULL, true);

        // 等待曝光完成
        while (!shutter_timer_fired) {
            tight_loop_contents();
        }

        // 额外延时确保定时器完成
        sleep_ms(100);

    } else if (mode == SHUTTER_FLASH) {
        // 闪光灯模式 (暂未完全实现)
        printf("Flash Mode!\r\n");

        // shutter_delay_x10 单位是 0.1ms，47ms = 470
        int gap = (int)shutter_delay_x10 - 470;
        if (gap < 0) gap = 0;

        shutter_open();
        sleep_ms(47);

        // 触发闪光灯 (FF 引脚)
        gpio_put(FF_PIN, 1);
        sleep_ms(1);
        gpio_put(FF_PIN, 0);
        sleep_ms(gap / 10);  // gap/10 转换回 ms

        shutter_close();

    } else if (mode == SHUTTER_BULB) {
        // B 门模式
        printf("B Mode!\r\n");
        shutter_open();
        sleep_ms(15);

        // 等待全按快门释放
        while (gpio_get(S1T_PIN) == 1) {
            sleep_ms(3);
        }

    } else if (mode == SHUTTER_TIME) {
        // T 门模式
        printf("T Mode!\r\n");
        shutter_open();

        // 等待按钮释放（按下=1，松开=0）
        while (gpio_get(S1T_PIN) == 1) {
            tight_loop_contents();
        }

        // 等待再次按下
        while (gpio_get(S1T_PIN) == 0) {
            sleep_ms(3);
        }
    }

    // 关闭快门，曝光结束
    shutter_close();
    printf("Shutter Closing!\r\n");
    sleep_ms(30);
    shutter_keep_closed();
    printf("Shutter Closed. Exposure Finished\r\n");
    sleep_ms(18);

    // 光圈归位
    if (mode == SHUTTER_FLASH) {
        aperture_disengage();
        printf("Aperture disengaged\r\n");
    }

    // 电机启动，开始吐片
    gpio_put(MOTOR_PIN, 1);
    printf("Motor Working for film ejection!\r\n");

    // 等待胶片检测 (S5 引脚)
    while (gpio_get(S5_PIN) == 1) {
        tight_loop_contents();
    }

    gpio_put(MOTOR_PIN, 0);
    shutter_open();
    printf("Film ejection complete!\r\n");
}
