#include "camera_main.h"
#include "PIN.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "camera";

TaskHandle_t control_task_handle = NULL;

void control_task(void *pvParameters)
{
    static bool test_led_level;
    while (1) {
        ESP_LOGI(TAG, "Control loop running on Core 1");
        vTaskDelay(pdMS_TO_TICKS(1000));
        test_led_level = !test_led_level;
        gpio_set_level(LED_TEST_PIN, test_led_level);
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
