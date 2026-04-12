# SX-70 Mk2 C SDK 重构计划

## 项目目标
将原有的 MicroPython 相机控制器代码（E:\Develop\SX-70-Mk2\Code\MainControllerCode）重构为 C 语言实现（基于 Raspberry Pi Pico SDK），**主要提升快门定时器精度**。

---

## 一、现有系统分析

### 1.1 核心功能模块
| 模块 | 功能 | MicroPython 实现 |
|------|------|------------------|
| **OLED 显示** | 128x32 SSD1306 屏幕，显示快门速度/模式/测光值 | `ssd1306.py` I2C 通信 |
| **测光系统** | TSL2561 光照传感器，自动曝光计算 | `tsl2561.py` I2C 通信 |
| **输入控制** | PCF8575 IO 扩展 + 编码器旋钮 + 3D 按键 | `pcf8575.py` |
| **快门控制** | PWM 驱动快门电磁铁，Timer 控制曝光时间 | `machine.PWM` + `machine.Timer` |
| **光圈控制** | PWM 驱动光圈电磁铁 | `machine.PWM` |
| **马达控制** | 驱动反光板升降/胶片吐出 | `machine.Pin` |
| **闪光灯** | FF 线触发闪光灯 | `machine.Pin` |

### 1.2 快门速度档位
```
ev6 = 1050ms (1s)      ev11 = 60ms  (1/30)
ev7 = 540ms  (1/2)     ev12 = 45ms  (1/60)
ev8 = 300ms  (1/4)     ev13 = 37ms  (1/125)
ev9 = 175ms  (1/8)     ev14 = 32ms  (1/250)
ev10 = 97ms  (1/15)    ev15 = 25ms  (1/500)
                        ev16 = 23ms  (1/1000)
                        ev17 = 22ms  (1/2000)
```

### 1.3 拍摄模式
- **A 档 (Auto)**: 自动测光决定快门速度
- **B 档 (Bulb)**: 按住快门按钮期间保持开启
- **T 档 (Time)**: 长曝光模式 (1 秒 - 4 分钟)
- **M 档 (Manual)**: 手动选择快门速度 (1/2s - 1/2000s)
- **自拍定时**: 3s/5s/10s 延时拍摄

### 1.4 原系统引脚定义 (MicroPython)
```
I2C0 (测光):  SCL=GPIO21, SDA=GPIO20
I2C1 (外接):  SCL=GPIO19, SDA=GPIO18

输出:
  快门 PWM:   GPIO9
  光圈 PWM:   GPIO17
  马达：      GPIO5
  LED 黄：     GPIO12
  LED 蓝：     GPIO13
  FF 闪光：   GPIO11
  S1F 对焦：  GPIO22

输入:
  S1F (半按): GPIO2
  S1T (全按): GPIO1
  S2 (闪光检测): GPIO14
  S3 (反光板位置): GPIO7
  S5 (胶片检测): GPIO6
```

---

## 二、重构任务清单

### 阶段 1: 项目基础架构
- [ ] **1.1** 创建模块化目录结构
  ```
  src/
    main.c              # 程序入口
    camera/
      camera.h/c        # 相机主控制类
      shutter.h/c       # 快门控制（高精度定时器）
      aperture.h/c      # 光圈控制
      motor.h/c         # 马达控制
    drivers/
      ssd1306.h/c       # OLED 显示驱动
      tsl2561.h/c       # 测光传感器驱动
      pcf8575.h/c       # IO 扩展芯片驱动
    ui/
      menu.h/c          # 菜单系统
      display_mgr.h/c   # 显示管理
    utils/
      debounce.h/c      # 按键消抖
      eeprom_mgr.h/c    # 参数存储
  ```
- [ ] **1.2** 更新 CMakeLists.txt 添加源文件
- [ ] **1.3** 配置 pico_sdk 必要组件 (I2C, PWM, Timer, GPIO)

### 阶段 2: 硬件驱动层
- [ ] **2.1** I2C 驱动封装 (`drivers/i2c_wrapper.h/c`)
  - 封装 I2C0/I2C1 初始化
  - 提供统一的读写接口
- [ ] **2.2** SSD1306 OLED 驱动 (`drivers/ssd1306.h/c`)
  - I2C 通信初始化
  - 命令/数据传输
  - 帧缓冲管理 (1KB buffer)
  - 图形 API: 画线/画矩形/文字显示
- [ ] **2.3** TSL2561 测光驱动 (`drivers/tsl2561.h/c`)
  - 增益自动切换
  - 多次采样滤波
  - Lux 值计算
- [x] **2.4** PCF8575 驱动 (`drivers/pcf8575.h/c`)
  - I2C 地址配置
  - 16 位 GPIO 读写
  - 中断支持（可选）
- [x] **2.5** LED 指示灯驱动 (`drivers/led.h/c`)
  - 黄灯 (GPIO12) 控制
  - 蓝灯 (GPIO13) 控制
  - ISO 指示逻辑
- [x] **2.6** SSD1306 OLED 驱动 (`drivers/ssd1306.h/c`)
  - I2C 通信初始化
  - 帧缓冲管理 (1KB buffer)
  - 显示控制：开关/对比度
  - 图形 API: 画点/画线/画矩形/文字显示

### 阶段 3: 核心控制模块 ⭐
- [ ] **3.1** 高精度快门定时器 (`camera/shutter.h/c`) **【核心优先级最高】**
  - 使用 RP2040 硬件定时器 (精度 1μs)
  - 替代原 `machine.Timer` (1ms 精度)
  - 实现 API:
    ```c
    shutter_init();
    shutter_open();
    shutter_close();
    shutter_set_exposure_time_us(uint32_t us);  // 微秒级设置
    shutter_trigger_exposure(void (*callback)(void)); // 硬件定时关闭
    ```
  - 支持曝光时间范围：22ms - 4 分钟
  - 目标精度：±50μs (原系统约±5ms)

- [ ] **3.2** 光圈控制 (`camera/aperture.h/c`)
  - PWM 驱动 (70kHz)
  - 到位/归位控制
- [ ] **3.3** 马达控制 (`camera/motor.h/c`)
  - GPIO 输出
  - 位置反馈检测 (S3/S5 引脚)
- [ ] **3.4** LED/闪光灯控制 (`camera/indicator.h/c`)
  - GPIO 输出管理

### 阶段 4: UI 与输入
- [ ] **4.1** 编码器输入处理 (`ui/encoder.h/c`)
  - PCF8575 读取
  - 格雷码解码 (主旋钮/副旋钮)
- [ ] **4.2** 3D 按键处理 (`ui/button3d.h/c`)
  - 消抖处理 (100ms)
  - 短按/长按识别
  - 菜单导航
- [ ] **4.3** 菜单系统 (`ui/menu.h/c`)
  - 状态机实现
  - 模式切换：A/T/B/M/自拍
- [ ] **4.4** 显示管理 (`ui/display_mgr.h/c`)
  - 界面刷新逻辑
  - 快门速度显示
  - 测光值显示

### 阶段 5: 相机主逻辑
- [ ] **5.1** 测光算法 (`camera/metering.h/c`)
  - TSL2561 数据读取
  - 多次采样平均
  - 曝光参数映射表
- [ ] **5.2** 拍摄流程控制 (`camera/exposure.h/c`)
  - 状态机管理
  - 拍摄时序控制:
    1. 关闭快门 (30ms)
    2. 升起反光板 (等待 S3 信号)
    3. 光圈就位 (闪光模式)
    4. 自拍延时 (可选)
    5. 开启快门 + 定时关闭
    6. 光圈归位
    7. 马达吐片 (等待 S5 信号)
- [ ] **5.3** 闪光灯支持 (`camera/flash.h/c`)
  - FF 线触发
  - S2 引脚检测
- [ ] **5.4** 主循环 (`main.c`)
  - 初始化
  - 事件循环
  - 状态同步

### 阶段 6: 参数存储与调试
- [ ] **6.1** EEPROM 管理 (`utils/eeprom_mgr.h/c`)
  - ISO 设置存储
  - 用户偏好保存
  - 使用 Pico Flash
- [ ] **6.2** 调试输出 (`utils/debug.h/c`)
  - UART 打印
  - 日志级别控制

---

## 三、关键技术点

### 3.1 高精度定时器实现方案
```c
// 使用 RP2040 硬件定时器 (精度 1μs)
#include "hardware/timer.h"

static alarm_id_t shutter_alarm_id;

int64_t shutter_close_callback(alarm_id_t id, void *user_data) {
    shutter_close();
    return 0;
}

void shutter_start_exposure(uint32_t exposure_us) {
    shutter_open();
    shutter_alarm_id = add_alarm_in_us(exposure_us, shutter_close_callback, NULL, true);
}
```

### 3.2 原 MicroPython Timer 精度分析
- `machine.Timer` 最小分辨率：1ms
- 实际误差：±5-10ms (RTOS 调度影响)
- 对 1/1000s (1ms) 档位影响显著

### 3.3 C SDK 改进目标
- 使用硬件定时器：精度提升至 1μs
- 理论误差：±50μs 以内
- 快门速度准确性提升 20 倍以上

---

## 四、开发优先级

1. **P0**: 基础框架 + 高精度快门定时器
2. **P1**: I2C 驱动 (SSD1306/TSL2561/PCF8575)
3. **P2**: 测光 + 曝光控制主逻辑
4. **P3**: UI 菜单系统
5. **P4**: 闪光灯/自拍等扩展功能

---

## 五、测试验证计划

### 5.1 单元测试
- [ ] 定时器精度测试 (示波器测量)
- [ ] I2C 设备通信测试
- [ ] 按键消抖测试

### 5.2 集成测试
- [ ] 完整拍摄流程测试
- [ ] 各快门速度实际曝光测试
- [ ] 测光准确性测试

### 5.3 对比测试
| 测试项 | MicroPython | C SDK | 目标改善 |
|--------|-------------|-------|----------|
| 1/1000s 实际时间 | ~1.5ms | 1.0±0.05ms | 精度 30x |
| 1/250s 实际时间 | ~4.5ms | 4.0±0.05ms | 精度 10x |
| 最长曝光 4 分钟 | 支持 | 支持 | 持平 |

---

## 六、进度追踪

- [ ] 阶段 1 完成：____-__-__
- [ ] 阶段 2 完成：____-__-__
- [ ] 阶段 3 完成：____-__-__
- [ ] 阶段 4 完成：____-__-__
- [ ] 阶段 5 完成：____-__-__
- [ ] 阶段 6 完成：____-__-__

---

*创建时间：2026-04-12*
