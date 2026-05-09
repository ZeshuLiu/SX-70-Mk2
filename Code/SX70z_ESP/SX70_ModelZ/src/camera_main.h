#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/** 控制任务句柄，供外部挂起/恢复 */
extern TaskHandle_t control_task_handle;

/** 相机控制任务 — 跑在 Core 1 */
void control_task(void *pvParameters);

/** OTA 期间挂起控制任务，避免 Flash 冲突 */
void camera_pause(void);

/** OTA 完成后恢复控制任务 */
void camera_resume(void);
