#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "drivers/pcf8575.h"
#include "drivers/ssd1306.h"
#include "drivers/font.h"
#include "drivers/fonts/font5x8.h"
#include "drivers/led.h"
#include "pins.h"

// 设备实例
static pcf8575_t pcf;
static ssd1306_t oled;

// 快门速度表 (Python 程序中的 M_CMD_Dict)
// ev6=1s, ev7=1/2, ev8=1/4, ev85=1/6, ev9=1/8, ev95=1/10, ev10=1/15, ev105=1/20
// ev11=1/30, ev115=1/45, ev12=1/60, ev125=1/90, ev13=1/125, ev135=1/180
// ev14=1/250, ev145=1/360, ev15=1/500, ev16=1/1000, ev17=1/2000
static const char *shutter_speeds[] = {
    "1s", "1/2", "1/3", "1/4", "1/6", "1/8", "1/10", "1/15", "1/20",
    "1/30", "1/45", "1/60", "1/90", "1/125", "1/180", "1/250", "1/360", "1/500", "1/1000", "1/2000"
};
#define SHUTTER_SPEED_COUNT (sizeof(shutter_speeds)/sizeof(shutter_speeds[0]))

// 相机状态 - 对应 Python Button3D 类
typedef struct {
    uint8_t menu;           // 菜单层级：0=AUTO, 1=BULB, 2=TIME, 3=MANUAL, 10=自拍
    uint8_t m_pos;          // M 档快门速度索引
    uint8_t self_timer_ind; // 自拍定时索引
    char cam_mode[6];       // 当前模式字符串
} camera_state_t;

static camera_state_t g_state = {0, 0, 0};

// 3D 按键状态 - 完全对应 Python
typedef struct {
    char old_button_value[4];   // 上次按键状态 "111"
    uint32_t debounce_last;     // 上次防抖时间
    uint32_t push_down_start;   // 按下键开始时间
} button3d_state_t;

static button3d_state_t g_btn3d = {"111", 0, 0};

// 自拍定时列表 (秒)
static const uint8_t timer_list[] = {0, 3, 5, 10};
#define TIMER_LIST_SIZE 4

void enter_bootloader() {
    reset_usb_boot(0, 0);
}

// 读取 PCF8575 的 3D 按键引脚 (对应 Python read_enc)
// 返回 3 位字符串："下上按"
void read_3d_button_pins(char *result) {
    uint16_t state = pcf8575_read(&pcf);

    result[0] = ((state >> PCF_BUTTON3D_DOWN) & 1) ? '1' : '0';
    result[1] = ((state >> PCF_BUTTON3D_UP) & 1) ? '1' : '0';
    result[2] = ((state >> PCF_BUTTON3D_PUSH) & 1) ? '1' : '0';
    result[3] = '\0';
}

// 更新相机模式显示
void update_mode_display() {
    if (g_state.menu == 0) {
        snprintf(g_state.cam_mode, sizeof(g_state.cam_mode), "AUTO");
    } else if (g_state.menu == 1) {
        snprintf(g_state.cam_mode, sizeof(g_state.cam_mode), "B");
    } else if (g_state.menu == 2) {
        snprintf(g_state.cam_mode, sizeof(g_state.cam_mode), "T");
    } else if (g_state.menu == 3) {
        snprintf(g_state.cam_mode, sizeof(g_state.cam_mode), "%s", shutter_speeds[g_state.m_pos]);
    } else if (g_state.menu == 10) {
        snprintf(g_state.cam_mode, sizeof(g_state.cam_mode), "---");
    }
}

// 下键按下回调
void down_button_call() {
    if (g_state.menu == 3) {  // M 档
        if (g_state.m_pos == 0) {
            g_state.m_pos = SHUTTER_SPEED_COUNT - 1;
        } else {
            g_state.m_pos--;
        }
    }
    update_mode_display();
}

// 上键按下回调
void up_button_call() {
    if (g_state.menu == 3) {  // M 档
        g_state.m_pos = (g_state.m_pos + 1) % SHUTTER_SPEED_COUNT;
    } else if (g_state.menu == 10) {  // 自拍定时
        g_state.self_timer_ind = (g_state.self_timer_ind + 1) % TIMER_LIST_SIZE;
    }
    update_mode_display();
}

// 短按"按下"键回调
void push_button_short() {
    g_state.menu = (g_state.menu + 1) % 4;  // 0->1->2->3->0
    update_mode_display();
}

// 长按"按下"键回调
void push_button_long() {
    if (g_state.menu < 10) {
        g_state.menu = 10;  // 进入自拍定时
    } else {
        g_state.menu = 0;   // 返回 AUTO
    }
    update_mode_display();
}

// 3D 按键处理 (完全对应 Python update 函数)
void button3d_handler() {
    char bt[4];
    read_3d_button_pins(bt);

    // 防抖检查 (100ms)
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - g_btn3d.debounce_last < 100) {
        return;
    }

    // 下键：下降沿触发 (1->0)
    if (g_btn3d.old_button_value[0] == '1' && bt[0] == '0') {
        down_button_call();
    }

    // 上键：下降沿触发 (1->0)
    if (g_btn3d.old_button_value[1] == '1' && bt[1] == '0') {
        up_button_call();
    }

    // 按下键：按下时记录时间 (1->0)
    if (g_btn3d.old_button_value[2] == '1' && bt[2] == '0') {
        g_btn3d.push_down_start = current_time;
    }

    // 按下键：松开时判断长短按 (0->1)
    if (g_btn3d.old_button_value[2] == '0' && bt[2] == '1') {
        uint32_t duration = current_time - g_btn3d.push_down_start;
        if (duration > 1000) {
            push_button_long();
        } else {
            push_button_short();
        }
    }

    // M 档时更新显示
    if (g_state.menu == 3) {
        snprintf(g_state.cam_mode, sizeof(g_state.cam_mode), "%s", shutter_speeds[g_state.m_pos]);
    }

    // 保存状态
    strncpy(g_btn3d.old_button_value, bt, 3);
    g_btn3d.debounce_last = current_time;
}

// 绘制显示帧 (对应 Python showFrame)
void show_frame() {
    ssd1306_clear(&oled);

    // 顶部状态栏 (黑底白字) - 先填充白色背景
    ssd1306_fill_rect(&oled, 0, 0, 128, 11);

    // 用黑色填充文字区域，形成反色效果
    ssd1306_clear_rect(&oled, 1, 1, 48, 9);   // 模式显示区域
    ssd1306_clear_rect(&oled, 52, 1, 28, 9);  // ISO 显示区域

    // 分隔线 (黑色)
    ssd1306_clear_rect(&oled, 51, 0, 1, 11);
    ssd1306_clear_rect(&oled, 82, 0, 1, 11);

    // 模式显示
    ssd1306_draw_str(&oled, 10, 2, g_state.cam_mode, &font5x8_font);

    // ISO 显示
    ssd1306_draw_str(&oled, 55, 2, "600", &font5x8_font);

    // 快门速度大字
    ssd1306_draw_str(&oled, 8, 18, shutter_speeds[g_state.m_pos], &font5x8_font);

    ssd1306_show(&oled);
}

// 初始化 PCF8575 和 OLED (共用 I2C1)
void pcf8575_init_system() {
    i2c_init(I2C_PORT_PLUG, 400 * 1000);
    gpio_set_function(I2C_PIN_PLUG_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_PIN_PLUG_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_PIN_PLUG_SDA);
    gpio_pull_up(I2C_PIN_PLUG_SCL);

    int ret = pcf8575_init(&pcf, I2C_PORT_PLUG, PCF8575_I2C_ADDR, 0xFFFF);
    printf("PCF8575 @ 0x20: %s\r\n", ret == 0 ? "OK" : "FAIL");
}

// 初始化 OLED (128x32)
void oled_init_system() {
    i2c_init(I2C_PORT_PLUG, 400 * 1000);
    gpio_set_function(I2C_PIN_PLUG_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_PIN_PLUG_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_PIN_PLUG_SDA);
    gpio_pull_up(I2C_PIN_PLUG_SCL);

    bool ret = ssd1306_init(&oled, 128, 32, 0x3C, I2C_PORT_PLUG, false);
    printf("OLED @ 0x3C: %s\r\n", ret ? "OK" : "FAIL");
}

int main() {
    stdio_init_all();
    sleep_ms(3000);

    printf("\r\n=== SX-70 Mk2 启动 ===\r\n");

    // 初始化
    led_init();
    pcf8575_init_system();
    oled_init_system();

    // 初始化状态
    update_mode_display();

    // 显示欢迎画面
    ssd1306_clear(&oled);
    ssd1306_draw_str(&oled, 10, 10, "SX-70 Mk2", &font5x8_font);
    ssd1306_draw_str(&oled, 5, 22, "Starting...", &font5x8_font);
    ssd1306_show(&oled);
    sleep_ms(1000);

    button3d_handler();
    // 打印按键状态
    char btn[4];
    read_3d_button_pins(btn);
    if (btn[0]=='0'|| btn[1]=='0' || btn[2]=='0')
    {
        enter_bootloader();
    }

    // 主循环
    while (true) {
        button3d_handler();
        show_frame();

        // 打印按键状态
        read_3d_button_pins(btn);
        printf("3D Button: up=%c down=%c mid=%c | Menu=%d Mode=%s\r\n",
                btn[0], btn[1], btn[2], g_state.menu, g_state.cam_mode);
        // sleep_ms(200);
    }
}
