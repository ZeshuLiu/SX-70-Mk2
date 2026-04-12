# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

SX-70 Mk2 相机控制器 - 基于 Raspberry Pi Pico 的胶片相机控制系统，从 MicroPython 重构为 C 语言以提升快门定时器精度（目标：±50μs）。

## 构建命令

```bash
# 进入构建目录
cd build

# 配置项目（首次）
cmake ..

# 编译
cmake --build .

# 生成 UF2 文件：build/SX70Mk2.uf2
```

## 项目结构

```
SX70Mk2/
├── SX70Mk2.c           # 主程序入口
├── CMakeLists.txt      # CMake 构建配置
├── TODO.md             # 详细开发任务清单
├── src/
│   ├── pins.h          # 引脚定义总表
│   └── drivers/
│       ├── pcf8575.h/c # I2C IO 扩展芯片驱动 (16 位 GPIO)
│       ├── ssd1306.h/c # OLED 显示屏驱动 (128x32)
│       └── led.h/c     # LED 指示灯驱动 (GPIO12/13)
```

## 硬件架构

### I2C 设备
- **I2C0** (GPIO20/21): TSL2561 测光传感器
- **I2C1** (GPIO18/19): PCF8575 IO 扩展 + SSD1306 OLED

### 关键引脚
- 快门 PWM: GPIO9
- 光圈 PWM: GPIO17
- 马达：GPIO5
- 按键输入：GPIO1/2/6/7/14
- LED: GPIO12(黄), GPIO13(蓝)

### PCF8575 连接
- I2C 地址：0x20
- P2: Button3D 按键
- P0/P1: 编码器旋钮（预留）

## 开发优先级

1. **P0**: 高精度快门定时器 (`camera/shutter.h/c`) - 使用 RP2040 硬件定时器
2. **P1**: 驱动层 - TSL2561 测光、编码器输入
3. **P2**: 测光算法 + 曝光控制主逻辑
4. **P3**: UI 菜单系统
5. **P4**: 闪光灯/自拍功能

## 重要注意事项

1. **USB 串口**: Pico CDC 串口需要在电脑端启用硬件流控 (RTS/CTS) 才能正常输出
2. **Bootloader**: 启动时检测 Button3D，按下则进入 UF2 Bootloader
3. **参考代码**: 原 MicroPython 实现在 `E:\Develop\SX-70-Mk2\Code\MainControllerCode`

## 待完成模块

详见 `TODO.md`，核心模块包括：
- `camera/shutter.h/c` - 高精度快门控制
- `camera/aperture.h/c` - 光圈 PWM 控制
- `camera/motor.h/c` - 马达控制
- `drivers/tsl2561.h/c` - 测光传感器
- `ui/encoder.h/c` - 编码器输入处理
- `ui/menu.h/c` - 菜单系统
