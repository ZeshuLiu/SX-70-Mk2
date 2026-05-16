#include "camera_main.h"
#include "display_manager.h"
#include "fonts/font5x8.h"
#include "opt4001.h"
#include "ssd1306.h"
#include "pcf8575.h"
#include "PIN.h"
#include "esp_log.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"

static const char *TAG = "camera";

#define HAS_FOCUS 0  // TODO: 对焦功能待实现

TaskHandle_t control_task_handle = NULL;

ssd1306_t display;
pcf8575_t gpio_expander;

static TaskHandle_t shutter_task_handle = NULL;

/* ---- us 级精确定时器（GPTimer one-shot alarm + ISR, Core 1 同核） ---- */
static volatile bool g_timer_done;
static gptimer_handle_t g_delay_timer;

static bool IRAM_ATTR delay_timer_cb(gptimer_handle_t timer,
                                      const gptimer_alarm_event_data_t *edata,
                                      void *user_ctx)
{
    g_timer_done = true;
    return false;
}

static gptimer_alarm_config_t g_alarm_cfg = {
    .alarm_count = 0,
    .reload_count = 0,
    .flags.auto_reload_on_alarm = false,
};

static void delay_us(uint32_t us)
{
    gptimer_stop(g_delay_timer);
    gptimer_set_raw_count(g_delay_timer, 0);
    g_alarm_cfg.alarm_count = us;
    gptimer_set_alarm_action(g_delay_timer, &g_alarm_cfg);
    g_timer_done = false;
    gptimer_start(g_delay_timer);
    while (!g_timer_done) {
        __asm__("nop");
    }
}

camera_state_t camera_state = {
    .if_display = false,
    .metering = {
        .last_lux = 0.0f,
    },
    .button = {
        .available = false,
        .old_value = "111",
        .debounce_last = 0,
        .push_down_start = 0,
    },
    .menu = 0,
    .cam_mode = "AUTO",
    .shut_mode = '1',
    .shutter_speed = 0,
    .test_led_level = false,
};

/* ---- 快门速度表（移植自 SX70Mk2 metering.c） ---- */
#define SHUTTER_SPEED_COUNT 22

static const char *shutter_speeds[] = {
    "1s", "1/2", "1/3", "1/4", "1/6", "1/8", "1/10", "1/15", "1/20",
    "1/30", "1/45", "1/60", "1/90", "1/125", "1/180", "1/250", "1/360",
    "1/500", "1/1000", "1/2000A", "1/2000B", "1/2000C"
};

// 快门时间 (0.1ms 单位)，对应上表
static const uint16_t shutter_times_x10[] = {
    10500, 5400, 4700, 3000, 2900, 1750, 1360, 970, 790,
    600, 520, 450, 410, 370, 340, 320, 280, 250, 230,
    225, 220, 215
};

const char *get_shutter_speed(uint8_t index)
{
    if (index >= SHUTTER_SPEED_COUNT) index = SHUTTER_SPEED_COUNT - 1;
    return shutter_speeds[index];
}

uint16_t get_shutter_time_x10(uint8_t index)
{
    if (index >= SHUTTER_SPEED_COUNT) index = SHUTTER_SPEED_COUNT - 1;
    return shutter_times_x10[index];
}

// S1 按键去抖计数
#define S1_DEBOUNCE_COUNT 5
static uint8_t s1f = 0;
static uint8_t s1t = 0;

static void debounce_read_s1pin(void)
{
    if (gpio_get_level(S1T_PIN) == 0) {
        s1t = (s1t == 0) ? s1t : s1t - 1;
    } else {
        s1t = S1_DEBOUNCE_COUNT;
    }
    // S1F 半按对焦——当前硬件可能没有，预留
}

/* ---- 测光任务（低优先级，1s 周期） ---- */
static void metering_task(void *pvParameters)
{
    esp_err_t ret = opt4001_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OPT4001 init failed: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "OPT4001 sensor initialized");

    while (1) {
        float lux;
        if (opt4001_read_lux(&lux) == ESP_OK) {
            camera_state.metering.last_lux = lux;
            ESP_LOGD(TAG, "OPT4001: %.4f lux", lux);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ---- 快门任务（移植 SX70Mk2 shutter_expose 完整时序） ---- */
static void shutter_task(void *pvParameters)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "Taking picture...");

        // 检查是否在拍摄模式（menu 10 = 自拍定时）
        if (camera_state.menu == 10) {
            ESP_LOGI(TAG, "Not in shooting mode (self-timer)");
            continue;
        }

#if HAS_FOCUS
        if (!camera_state.if_focused) {
            do_focus(&camera_state.shut_mode, &camera_state.shutter_speed);
        }
#else
        // TODO: 对焦功能待实现（需移植 do_focus / S1F_FBW_PIN）
#endif

        uint16_t shutter_delay_x10 = get_shutter_time_x10(camera_state.shutter_speed);
        char mode = camera_state.shut_mode;

        // 关闭 LED
        gpio_set_level(LED1_PIN, 1);

        // ---- 1. 关闭快门 ----
        ESP_LOGD(TAG, "Shutter close");
        gpio_set_level(SOL1_PIN, 1);  // shutter_close → 高电平关闭快门
        delay_us(30000);               // 30ms
        ESP_LOGD(TAG, "Shutter closed");

        // ---- 2. 电机启动，反光板上升 ----
        gpio_set_level(MOTOR_PIN, 1);
        ESP_LOGD(TAG, "Motor start (mirror up)");

        // 等待反光板就位 (S3 变高)
        while (gpio_get_level(S3_PIN) == 0) {
            delay_us(100);
        }
        gpio_set_level(MOTOR_PIN, 0);
        ESP_LOGD(TAG, "Motor stopped");

        // ---- 3. Y Delay ----
        if (mode == '0') {  // SHUTTER_FLASH
            gpio_set_level(SOL2_PIN, 1);  // 光圈就位
            ESP_LOGD(TAG, "Aperture engaged");
        }
        delay_us(18000);  // 18ms Y delay

        // ---- 4. 曝光 ----
        ESP_LOGD(TAG, "Exposure: mode=%c, delay=%d (0.1ms)", mode, shutter_delay_x10);

        if (mode == '1') {  // SHUTTER_NORMAL
            gpio_set_level(SOL1_PIN, 0);  // shutter_open
            delay_us((uint32_t)shutter_delay_x10 * 100);  // 0.1ms → us
            gpio_set_level(SOL1_PIN, 1);  // shutter_close
            delay_us(100000);  // 100ms

        } else if (mode == '0') {  // SHUTTER_FLASH
            int gap = (int)shutter_delay_x10 - 470;
            if (gap < 0) gap = 0;

            gpio_set_level(SOL1_PIN, 0);  // shutter_open
            delay_us(47000);  // 47ms

            gpio_set_level(FF_PIN, 1);     // 触发闪光灯
            delay_us(1000);                // 1ms
            gpio_set_level(FF_PIN, 0);
            delay_us((uint32_t)gap * 100); // 剩余延时

            gpio_set_level(SOL1_PIN, 1);  // shutter_close

        } else if (mode == 'B') {  // SHUTTER_BULB
            gpio_set_level(SOL1_PIN, 0);  // shutter_open
            delay_us(15000);  // 15ms

            // 等待 S1T 释放
            while (gpio_get_level(S1T_PIN) == 0) {
                delay_us(3000);
            }

        } else if (mode == 'T') {  // SHUTTER_TIME
            gpio_set_level(SOL1_PIN, 0);  // shutter_open

            // 等待 S1T 释放
            while (gpio_get_level(S1T_PIN) == 0) {
                delay_us(100);
            }
            // 等待 S1T 再次按下
            while (gpio_get_level(S1T_PIN) != 0) {
                delay_us(3000);
            }
        }

        // ---- 5. 关闭快门，曝光结束 ----
        gpio_set_level(SOL1_PIN, 1);  // shutter_close
        ESP_LOGD(TAG, "Shutter closing");
        delay_us(30000);   // 30ms
        delay_us(18000);   // 18ms

        // ---- 6. 光圈归位 ----
        if (mode == '0') {  // SHUTTER_FLASH
            gpio_set_level(SOL2_PIN, 0);  // aperture_disengage
            ESP_LOGD(TAG, "Aperture disengaged");
        }

        // ---- 7. 电机启动吐片 ----
        gpio_set_level(MOTOR_PIN, 1);
        ESP_LOGD(TAG, "Motor start (film ejection)");

        // 等待 S5 变低（胶片检测）
        while (gpio_get_level(S5_PIN) != 0) {
            delay_us(100);
        }

        gpio_set_level(MOTOR_PIN, 0);
        gpio_set_level(SOL1_PIN, 0);  // shutter_open
        ESP_LOGI(TAG, "Film ejection complete");

        // ---- 8. 等待 S1T 释放，防止连拍 ----
        while (gpio_get_level(S1T_PIN) == 0) {
            delay_us(10000);  // 10ms
        }
    }
}

void control_task(void *pvParameters)
{
    /* ---- SSD1306 OLED（I2C1, 0x3C, 128x64） ---- */
    if (ssd1306_init(&display, 128, 64, 0x3C, I2C_NUM_1, false)) {
        ssd1306_clear(&display);
        ssd1306_draw_str(&display, 0, 0, "SX70z Ready", &font5x8_font);
        ssd1306_show(&display);
        camera_state.if_display = true;
        ESP_LOGI(TAG, "SSD1306 initialized");
    } else {
        ESP_LOGE(TAG, "SSD1306 init failed");
    }

    /* ---- PCF8575 GPIO 扩展（I2C1, 0x20） ---- */
    if (pcf8575_init(&gpio_expander, I2C_NUM_1, 0x20, 0xFFFF) == 0) {
        ESP_LOGI(TAG, "PCF8575 initialized");
        camera_state.button.available = true;
    } else {
        ESP_LOGE(TAG, "PCF8575 init failed");
        camera_state.button.available = false;
    }

    /* ---- 启动测光任务（Core 1，低优先级） ---- */
    xTaskCreatePinnedToCore(metering_task, "metering", 2048, NULL,
                            METERING_TASK_PRIO, NULL, 1);

    /* ---- 初始化 us 级精确定时器（GPTimer，ISR 绑定 Core 1） ---- */
    gptimer_config_t tcfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,  // 1MHz → 1μs
        .intr_priority = 1,
    };
    gptimer_new_timer(&tcfg, &g_delay_timer);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = delay_timer_cb,
    };
    gptimer_register_event_callbacks(g_delay_timer, &cbs, NULL);
    gptimer_enable(g_delay_timer);

    /* ---- 启动快门任务（Core 1，最高优先级，平时阻塞） ---- */
    xTaskCreatePinnedToCore(shutter_task, "shutter", 2048, NULL,
                            SHUTTER_TASK_PRIO, &shutter_task_handle, 1);

    while (1) {
        // 闪光灯检测
        bool flash_connected = (gpio_get_level(S2_PIN) == 0);

        // S1 去抖读取
        debounce_read_s1pin();
        bool s1t_pressed = (s1t > 0);

        // S1T 全按快门 → 触发快门任务（参考 if s1t_pressed == 1）
        if (s1t_pressed) {
            if (camera_state.menu != 10) {
                xTaskNotifyGive(shutter_task_handle);
            }
        }

        display_show_frame(&camera_state, &display);

        ESP_LOGD(TAG, "S1T=%d Flash=%d Mode=%s LUX=%.2f",
                s1t_pressed, flash_connected,
                camera_state.cam_mode, camera_state.metering.last_lux);

        camera_state.test_led_level = !camera_state.test_led_level;
        gpio_set_level(LED1_PIN, camera_state.test_led_level);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void camera_pause(void)
{
    if (control_task_handle) {
        vTaskSuspend(control_task_handle);
        // 等待 Core 1 任务真正挂起后才返回
        while (eTaskGetState(control_task_handle) != eSuspended) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        ESP_LOGI(TAG, "Paused for OTA");
    }
}

void camera_resume(void)
{
    if (control_task_handle) {
        vTaskResume(control_task_handle);
        ESP_LOGI(TAG, "Resumed after OTA");
    }
}
