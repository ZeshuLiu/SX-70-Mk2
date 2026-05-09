# SX70_ModelZ 配置问题清单

> 芯片: ESP32-PICO-V3 (4MB Flash, 无 PSRAM) | ESP-IDF v5.5.2 | 分支: Working

---

## 1. OTA 回滚未启用 [高] ✅ 已修复
- ~~`CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE` 未设置~~
- 已手动启用

## 2. BLE 配对信息不持久化 [高] ✅ 已修复
- ~~`CONFIG_BT_NIMBLE_NVS_PERSIST` 未设置~~
- 已手动启用

## 3. 编译优化级别为 DEBUG [中] 🔧 开发阶段暂保留
- `CONFIG_COMPILER_OPTIMIZATION_DEBUG=y`
- 开发阶段保留 DEBUG，发布前切 -Os

## 4. main.c 完全为空 [严重] 🔧 进行中
- 编写初始化流程：NVS → WiFi → BLE Provisioning（非阻塞）→ 主控循环
- BLE 配网后保持连接，后续扩展数据/OTA 通道

## 5. WiFi 配网后 BLE 自动关闭 [中] ✅ 已修复
- ~~`CONFIG_WIFI_PROV_KEEP_BLE_ON_AFTER_PROV` 未设置~~
- 已手动启用

## 6. 分区表自定义文件名残留 [低]
- `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"` 但未启用 CUSTOM 模式
- 实际用内置 `partitions_two_ota.csv`，暂不影响

---

## 已完成
1. ✅ OTA 回滚
2. ✅ BLE NVS 持久化
3. ✅ BLE 配网后保持连接

## 待完成
- 🔧 main.c 初始化代码
- ⏳ BLE 自定义 Service（数据/控制 + OTA 传输）
- ⏳ OTA 升级逻辑
- ⏳ 编译优化（发布前）
- ⏳ 分区表清理
