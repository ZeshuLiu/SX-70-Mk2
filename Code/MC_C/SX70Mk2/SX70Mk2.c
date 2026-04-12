#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "drivers/pcf8575.h"
#include "pins.h"

// PCF8575 设备实例
static pcf8575_t pcf;

void enter_bootloader() {
    // 直接跳转到 USB UF2 Bootloader
    reset_usb_boot(0, 0);
}

// 初始化 PCF8575
void pcf8575_button_init() {
    // 初始化 I2C1 (外接控制器 I2C)
    i2c_init(I2C_PORT_PLUG, 400 * 1000);  // 400kHz

    // 设置 I2C1 引脚 (GPIO 18=SDA, GPIO 19=SCL)
    gpio_set_function(I2C_PIN_PLUG_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_PIN_PLUG_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_PIN_PLUG_SDA);
    gpio_pull_up(I2C_PIN_PLUG_SCL);

    // 初始化 PCF8575: Button3D(P2) 为输入
    int ret = pcf8575_init(&pcf, I2C_PORT_PLUG, PCF8575_I2C_ADDR, 0xFFFF);
    if (ret != 0) {
        printf("PCF8575 初始化失败!\r\n");
    } else {
        printf("PCF8575 初始化成功!\r\n");
    }
}

int main() {
    // 先初始化 stdio
    stdio_init_all();

    // 小延时等待 USB 枚举
    sleep_ms(200);

    printf("\r\n========================================\r\n");
    printf("       SX-70 Mk2 启动\r\n");
    printf("========================================\r\n");

    // 初始化 PCF8575 用于检测 Button3D
    i2c_init(I2C_PORT_PLUG, 400 * 1000);
    gpio_set_function(I2C_PIN_PLUG_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_PIN_PLUG_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_PIN_PLUG_SDA);
    gpio_pull_up(I2C_PIN_PLUG_SCL);

    // 初始化 PCF8575
    int ret = pcf8575_init(&pcf, I2C_PORT_PLUG, PCF8575_I2C_ADDR, 0xFFFF);
    if (ret != 0) {
        printf("PCF8575 初始化失败!\r\n");
    } else {
        printf("PCF8575 初始化成功!\r\n");
    }

    // 开机检测 Button3D，如果按下则进入 bootloader
    uint16_t state = pcf8575_read(&pcf);
    uint8_t button3d = (state >> PCF_P2) & 1;
    printf("\r\n开机检测：Button3D = %d\r\n", button3d);
    printf("PCF8575 原始值 = 0x%04X\r\n", state);

    if (button3d == 0) {  // 按键按下（低电平）
        printf("检测到 Button3D 按下，进入 Bootloader...\r\n");
        sleep_ms(500);
        enter_bootloader();
    }

    printf("正常启动...\r\n");
    printf("\r\n持续检测 Button3D...\r\n\r\n");

    while (true) {
        state = pcf8575_read(&pcf);
        button3d = (state >> PCF_P2) & 1;
        printf("Button3D = %d\r\n", button3d);
        sleep_ms(200);
    }
}
