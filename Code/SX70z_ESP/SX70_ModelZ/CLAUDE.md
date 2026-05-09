# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Polaroid SX-70 相机控制器，基于 ESP32-PICO-V3 (4MB Flash, 无 PSRAM)，运行 ESP-IDF v5.5.2。

- **硬件**: 自绘 PCB，ESP32-PICO-V3、声纳、闪光灯、快门/光圈控制、PCF8575 I2C GPIO 扩展、SSD1306 OLED、OPT4001 环境光传感器
- **当前分支**: `Working` — 在原有相机控制代码基础上新增 BLE + WiFi + OTA 功能
- **ESP-IDF**: `D:\ESPIDF\v5.5.2\esp-idf` / **工具链**: `D:\ESPIDF_TOOL`

## Build & Flash

```bash
idf.py build                      # 编译
idf.py -p <PORT> flash            # 烧录 (UART, QIO 80MHz)
idf.py -p <PORT> monitor          # 串口监视 (UART0, 115200 baud)
idf.py -p <PORT> erase-flash flash monitor
```

## Architecture

### 核心设计原则

- **双核隔离**: Core 0 跑所有非控制逻辑（WiFi/BLE/HTTP/日志），Core 1 只跑时序敏感的相机控制。**与控制无关的代码一律放 Core 0**
- **非阻塞初始化**: WiFi 配网、BLE 连接在后台任务运行，不阻塞主控
- **事件驱动**: WiFi/IP 状态变更通过 `esp_event` 回调通知
- **OTA 安全**: OTA 写 Flash 前挂起 Core 1 控制任务（`camera_pause`），完成后恢复或重启

### CPU 分配

```
Core 0:  WiFi 协议栈 + NimBLE BLE + HTTP Server (OTA Web) + 事件回调 + app_main
Core 1:  相机控制任务 — 声纳/快门/光圈/闪光灯（时序敏感，与射频中断隔离）
```

### 目录结构

```
SX70_ModelZ/
├── main/
│   ├── main.c              # WiFi/BLE 初始化，启动控制任务，触发 OTA Web
│   └── CMakeLists.txt      # REQUIRES: nvs_flash esp_wifi esp_event esp_netif wifi_provisioning src
├── src/
│   ├── CMakeLists.txt      # REQUIRES: esp_http_server app_update
│   ├── devinfo.h / .c      # 设备信息（序列号=芯片 MAC、软硬件版本）
│   ├── camera_main.h / .c  # 相机控制任务 (Core 1)，含 camera_pause/resume
│   ├── opt4001.h / .c      # OPT4001 环境光传感器驱动（I2C0, 0x44）
│   ├── ssd1306.h / .c      # SSD1306 OLED 显示驱动（I2C1）
│   ├── pcf8575.h / .c      # PCF8575 I2C GPIO 扩展（I2C1, 0x20-0x27）
│   ├── font.h              # 字体类型定义
│   ├── fonts/font5x8.h     # 5x8 像素字体
│   └── ota_web.h / .c      # HTTP 网页上传固件 OTA
├── components/             # ESP-IDF 标准组件目录（当前为空）
├── sdkconfig
└── CONFIG_ISSUES.md        # 配置问题跟踪清单
```

### 初始化流程

```
app_main() [Core 0]
  0. devinfo_init()                        — 读芯片 MAC 做序列号
  1. OTA 回滚检查                           — 若分区为 PENDING_VERIFY，调用
                                             esp_ota_mark_app_valid_cancel_rollback()
                                             确认固件有效（bootloader watchdog 要求）
  2. pin_init()                            — GPIO 初始化
  3. NVS 初始化                             — 存储 WiFi 凭据、BLE 绑定
  4. esp_netif + event loop                — 网络栈基础
  5. 注册 WiFi / IP / Provisioning 事件回调
  6. WiFi STA 启动 + 设置主机名 "SX70z"
  7. BLE Provisioning（非阻塞）             — 未配网则广播，已配网则直接连 WiFi
  8. xTaskCreatePinnedToCore(control_task, 1) — 启动 Core 1 控制任务
  9. app_main 空闲循环（打印 RSSI 等辅助信息）
```

### OTA 升级流程

```
IP_EVENT_STA_GOT_IP → ota_web_start()
  → 浏览器打开 http://<ESP32_IP>
  → 选 .bin 文件上传
  → POST /update:
       camera_pause()              ← 挂起 Core 1 控制任务
       esp_ota_begin()             ← 打开 ota_1 分区
       esp_ota_write() × N         ← 逐块写入 Flash
       esp_ota_end()               ← 校验
       esp_ota_set_boot_partition(ota_1)
       esp_ota_get_boot_partition() ← 验证启动分区已正确设置
       esp_restart()               ← 重启进入新固件
    错误路径: camera_resume()      ← 恢复 Core 1 控制任务

  重启后 → app_main 步骤 1 检测到 PENDING_VERIFY → 调用
  esp_ota_mark_app_valid_cancel_rollback() 确认新固件有效，回滚取消
```

### OTA 回滚机制

`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` 启用后：

1. `esp_ota_set_boot_partition()` 将新分区标记为 `PENDING_VERIFY`
2. Bootloader 启动新固件时设置 watchdog 定时器
3. **新固件必须在 watchdog 超时前调用 `esp_ota_mark_app_valid_cancel_rollback()`**，否则：
   - Bootloader 将当前分区标记为 `INVALID`
   - 下次重启自动回滚到上一个有效分区
4. 若新固件启动期间崩溃（LoadProhibited 等），bootloader 同样自动回滚

关键调用在 `app_main` 步骤 1，位于所有初始化之前，确保 watchdog 不会意外触发。

### BLE / WiFi 技术栈

| 组件 | 选型 |
|---|---|
| BLE 协议栈 | NimBLE (VHCI) |
| WiFi 配网 | BLE Provisioning (Proof of Possession) |
| OTA | WiFi HTTP 网页上传 |
| 分区表 | `partitions_two_ota_large.csv` (ota_0 + ota_1 各 1700KB) |

### sdkconfig 关键配置

- `CONFIG_IDF_TARGET="esp32"` — PICO-V3 在 IDF 中归类为 ESP32
- `CONFIG_BT_NIMBLE_ENABLED=y` — NimBLE 协议栈
- `CONFIG_PARTITION_TABLE_TWO_OTA_LARGE=y` — 双 OTA (各 1700KB)
- `CONFIG_WIFI_PROV_BLE_SEC_CONN=y` — BLE 安全配网
- `CONFIG_WIFI_PROV_KEEP_BLE_ON_AFTER_PROV=y` — 配网后保持 BLE
- `CONFIG_BT_NIMBLE_NVS_PERSIST=y` — BLE 配对持久化
- `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` — OTA 失败回滚
- `CONFIG_COMPILER_OPTIMIZATION_DEBUG=y` — 开发阶段用 DEBUG
- `CONFIG_ESPTOOLPY_FLASHMODE_QIO=y` / `CONFIG_ESPTOOLPY_FLASHFREQ_80M=y` — QIO 80MHz
- `CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y` — 默认日志级别 DEBUG
- BLE 设备名 `"SX70z"` / 广播名 `"SX70z_POP"` / PoP `"sx70z123"`
- WiFi 主机名 `"SX70z"`

### 日志级别约定

| 级别 | 用途 |
|---|---|
| `ESP_LOGE` | 错误（始终打印） |
| `ESP_LOGW` | 警告 |
| `ESP_LOGI` | 重要信息：WiFi 连接/断开、配网成功/失败、OTA 进度、设备信息 |
| `ESP_LOGD` | 调试细节：STA 启动、配网开始/结束、凭据收到、WiFi 未连接 |
| `ESP_LOGV` | 未启用 |

## Peripheral Hardware & I2C Bus

| 总线 | 引脚 | 速度 | 设备 |
|------|------|------|------|
| I2C0 | GPIO21/22 | 100kHz | OPT4001 (0x44) |
| I2C1 | GPIO7/8 | 400kHz | SSD1306 (0x3C), PCF8575 (0x20-0x27) |

### OPT4001 环境光传感器

- 挂 I2C_NUM_0（GPIO21/22），地址 0x44
- 初始化在 `control_task` (Core 1) 启动时调 `opt4001_init()`，此时 Core 0 已完成 `pin_init()` 初始化 I2C
- 自动量程模式，800ms 转换周期，连续采样 — 初始化后首次有效数据延时 900ms
- I2C 读时序：两次独立事务（写寄存器地址 → STOP → 读数据 → STOP），备选 Repeated Start 方案注释在代码中待验证
- 量程 0.001 ~ 2,200,000 lux，12 档硬件自动切换

### SSD1306 OLED

- 挂 I2C_NUM_1（GPIO7/8），默认地址 0x3C，400kHz
- 移植自 tapiocode 的 Pico 驱动，API 保留：`ssd1306_init/show/draw_str/draw_line/draw_rect/draw_circle`
- 本地帧缓冲 + `ssd1306_show()` 全量刷新到屏幕
- 字体使用 `font5x8_font`（5x8 像素，96 字符 ASCII）

### PCF8575 I2C GPIO 扩展

- 挂 I2C_NUM_1（GPIO7/8），地址 0x20-0x27
- 16 位 GPIO，方向可逐位配置（1=输入 0=输出）
- API：`pcf8575_init/read/write/write_pin/read_pin/set_input/set_output`

## Development Notes

- `src/` 通过 `EXTRA_COMPONENT_DIRS` 注册为独立组件，新增 .c 文件需在 `src/CMakeLists.txt` 的 SRCS 中添加
- 新增组件依赖时注意 CMake 组件名 vs 头文件名不一致（如 `esp_ota_ops.h` → `app_update`）
- `freertos` / `esp_mac` 等基础组件自动链接，无需声明 REQUIRES
- `.vscode/settings.json` 中 `IDF_TARGET` 固定为 `esp32`
- OTA 升级时必须先 `camera_pause()` 挂起 Core 1，避免 Flash 写冲突导致 LoadProhibited
- `CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y` 要求固件启动后调用 `esp_ota_mark_app_valid_cancel_rollback()`（在 `app_main` 步骤 1 中处理），否则 bootloader watchdog 会触发回滚
- 若 OTA 后重启仍运行旧固件，检查串口日志中的 "Running partition: xxx, state: x" 确认启动分区和回滚状态
- `CONFIG_ISSUES.md` 跟踪配置项待办
