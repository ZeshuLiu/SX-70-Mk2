#include "camera_main.h"
#include "opt4001.h"
#include "ssd1306.h"
#include "pcf8575.h"
#include "fonts/font5x8.h"
#include "PIN.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "camera";

TaskHandle_t control_task_handle = NULL;

ssd1306_t display;
pcf8575_t gpio_expander;

void control_task(void *pvParameters)
{
    /* ---- OPT4001 光照传感器（I2C0, 0x44） ---- */
    esp_err_t ret = opt4001_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "OPT4001 init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "OPT4001 sensor initialized");
    }

    /* ---- SSD1306 OLED（I2C1, 0x3C, 128x64） ---- */
    if (ssd1306_init(&display, 128, 64, 0x3C, I2C_NUM_1, false)) {
        ssd1306_clear(&display);
        ssd1306_draw_str(&display, 0, 0, "SX70z Ready", &font5x8_font);
        ssd1306_show(&display);
        ESP_LOGI(TAG, "SSD1306 initialized");
    } else {
        ESP_LOGE(TAG, "SSD1306 init failed");
    }

    /* ---- PCF8575 GPIO 扩展（I2C1, 0x20） ---- */
    if (pcf8575_init(&gpio_expander, I2C_NUM_1, 0x20, 0xFFFF) == 0) {
        ESP_LOGI(TAG, "PCF8575 initialized");
    } else {
        ESP_LOGE(TAG, "PCF8575 init failed");
    }

    static bool test_led_level;
    while (1) {
        ESP_LOGI(TAG, "S2=%d (flash: %s)", gpio_get_level(S2_PIN),
                gpio_get_level(S2_PIN) == 0 ? "attached" : "none");
        float lux;
        if (opt4001_read_lux(&lux) == ESP_OK) {
            ESP_LOGI(TAG, "OPT4001: %.4f lux", lux);
        }
        test_led_level = !test_led_level;
        gpio_set_level(LED1_PIN, test_led_level);
        vTaskDelay(pdMS_TO_TICKS(1000));
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
