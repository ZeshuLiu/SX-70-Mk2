/**
 * SX70_ModelZ — 主程序
 *
 * CPU 分配：
 *   Core 0: WiFi 协议栈 + NimBLE BLE + 事件处理（射频相关，必须在此核）
 *   Core 1: 相机控制任务（时序敏感，与射频中断隔离）
 *
 * 初始化流程（均在 Core 0 后台运行，不阻塞控制任务）：
 *   1. NVS           — 存储 WiFi 凭据、BLE 配对信息
 *   2. WiFi STA      — 启动 Station 模式
 *   3. BLE Provisioning — 若未配网则广播等待手机 App 发送凭据
 *   4. 启动控制任务    — Core 1，独立运行，不受 WiFi/BLE 状态影响
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "devinfo.h"
#include "camera_main.h"
#include "ota_web.h"

static const char *TAG = "main";

/* ================================================================
 *  WiFi 事件回调
 *
 *  由 ESP WiFi 任务调用，不占用 app_main 时间片。
 *  断线自动重连。
 * ================================================================ */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGD(TAG, "WiFi STA started");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // 断线自动重连，不阻塞主程序
        ESP_LOGI(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        static bool ota_started = false;
        if (!ota_started) {
            ota_started = true;
            ota_web_start();
        }
    }
}

/* ================================================================
 *  Provisioning 事件回调
 *
 *  BLE 配网流程的状态通知，仅做日志记录。
 *  配网失败时重启让用户重试。
 * ================================================================ */

static void prov_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base != WIFI_PROV_EVENT) return;

    switch (event_id) {
    case WIFI_PROV_START:
        ESP_LOGD(TAG, "Provisioning started");
        break;
    case WIFI_PROV_CRED_RECV:
        // 手机 App 已发送 WiFi 凭据
        ESP_LOGD(TAG, "WiFi credentials received");
        break;
    case WIFI_PROV_CRED_SUCCESS:
        // 凭据正确，WiFi 已连接
        ESP_LOGI(TAG, "Provisioning successful");
        break;
    case WIFI_PROV_CRED_FAIL:
        ESP_LOGI(TAG, "Provisioning failed, restarting...");
        esp_restart();
        break;
    case WIFI_PROV_END:
        // 配网流程结束（BLE 保持广播，不受影响）
        ESP_LOGD(TAG, "Provisioning ended");
        break;
    default:
        break;
    }
}

/* ================================================================
 *  app_main — 入口
 *
 *  所有初始化均为非阻塞调用，完成后进入主控循环。
 *  WiFi 配网、BLE 连接均在后台由各自的 FreeRTOS 任务处理。
 * ================================================================ */

void app_main(void)
{
    devinfo_init();
    ESP_LOGI(TAG, "SX70z starting...");
    ESP_LOGI(TAG, "Model: %s  SN: %s  SW: %d.%d.%d  HW: r%d",
             device.model, device.serial,
             device.sw_major, device.sw_minor, device.sw_patch,
             device.hw_rev);

    /* ---- 1. NVS 初始化 ----
     * 存储 WiFi 凭据（wifi_prov_mgr 用 "nvs" 分区）。
     * 首次使用或分区损坏时自动擦除重建。 */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* ---- 2. 网络栈初始化 ----
     * esp_netif + event loop 是 WiFi / IP 事件分发的基础设施。 */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* ---- 3. 注册事件回调 ----
     * 所有回调在独立上下文中执行，不阻塞 app_main。 */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                                                prov_event_handler, NULL));

    /* ---- 4. WiFi Station 初始化 ----
     * 仅 STA 模式，不开启 AP。WiFi 启动后若已配网则自动连接。 */
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(sta_netif, "SX70z");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* ---- 5. BLE Provisioning ----
     * 若 NVS 中无 WiFi 凭据，启动 BLE 广播等待手机 App 配网。
     * wifi_prov_mgr_start_provisioning() 立即返回，配网在后台进行。
     *
     * 安全参数：
     *   - security = WIFI_PROV_SECURITY_1 (Proof of Possession 密码保护)
     *   - pop = "sx70z123" (配网时需要输入的密码)
     *   - 设备名 = "SX70z_POP" (BLE 广播名)
     *
     * 配网完成后 BLE 保持开启（sdkconfig 已配置 KEEP_BLE_ON），
     * 后续可在此 NimBLE 实例上注册自定义 Service（数据/控制/OTA）。 */
    wifi_prov_mgr_config_t prov_config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
        .app_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
    };
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));

    bool provisioned = false;
    wifi_prov_mgr_is_provisioned(&provisioned);

    if (!provisioned) {
        ESP_LOGI(TAG, "Not provisioned, starting BLE provisioning...");
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
            security, "sx70z123", "SX70z_POP", NULL));
    } else {
        ESP_LOGI(TAG, "Already provisioned, connecting to WiFi...");
        wifi_prov_mgr_deinit();
        esp_wifi_connect();
    }

    /* ---- 6. 启动控制任务到 Core 1 ----
     * 控制逻辑与 WiFi/BLE 协议栈物理隔离，互不干扰时序。 */
    xTaskCreatePinnedToCore(control_task, "control", 4096, NULL, 5,
                            &control_task_handle, 1);
    ESP_LOGI(TAG, "Init done, control task running on Core 1");

    /* Core 0 辅助任务：打印 WiFi 信号强度等非控制相关工作 */
    while (1) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "WiFi RSSI: %d dBm", ap_info.rssi);
        } else {
            ESP_LOGD(TAG, "WiFi not connected");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
