#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "drivers/pcf8575.h"
#include "drivers/ssd1306.h"
#include "drivers/font.h"
#include "drivers/fonts/font5x8.h"
#include "drivers/led.h"
#include "drivers/tsl2561.h"
#include "pins.h"

// 设备实例
static pcf8575_t pcf;
static ssd1306_t oled;
static tsl2561_t lm;  // 测光表

// 快门速度表 (Python 程序中的 M_CMD_Dict)
// ev6=1s, ev7=1/2, ev8=1/4, ev85=1/6, ev9=1/8, ev95=1/10, ev10=1/15, ev105=1/20
// ev11=1/30, ev115=1/45, ev12=1/60, ev125=1/90, ev13=1/125, ev135=1/180
// ev14=1/250, ev145=1/360, ev15=1/500, ev16=1/1000, ev17=1/2000
static const char *shutter_speeds[] = {
    "1s", "1/2", "1/3", "1/4", "1/6", "1/8", "1/10", "1/15", "1/20",
    "1/30", "1/45", "1/60", "1/90", "1/125", "1/180", "1/250", "1/360",
    "1/500", "1/1000", "1/2000"
};
#define SHUTTER_SPEED_COUNT 20

// 相机状态 - 对应 Python Button3D 类
typedef struct {
    uint8_t menu;           // 菜单层级：0=AUTO, 1=BULB, 2=TIME, 3=MANUAL, 10=自拍
    uint8_t m_pos;          // M 档快门速度索引
    uint8_t self_timer_ind; // 自拍定时索引
    char cam_mode[6];       // 当前模式字符串
    float last_lux;         // 上次测光值 (浮点数)
    uint8_t auto_shutter_pos; // AUTO 档计算的快门位置
} camera_state_t;

static camera_state_t g_state = {0, 0, 0, "", 0, 0};

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

// 根据 LUX 值计算快门速度 (ISO 600)
// Python 代码：camera_driver.py light_meter() 函数
// 传入的 lux 已经是调整后的值：(lux_raw - 0.4) * 0.9
// 返回值是快门速度在 shutter_speeds 数组中的索引 (1-19 对应 ev7-ev17)
uint8_t calc_shutter_from_lux(float lux) {
    // Python 中的阈值判断 (完全对应)
    // lux <= 0.120 返回 ev7 (1/2s) - 对应索引 1
    if (lux <= 0.120f) return 1;   // ev7 (1/2s)
    if (lux <= 0.125f) return 2;   // ev75 (1/3s)
    if (lux <= 0.1385f) return 3;  // ev8 (1/4s)
    if (lux <= 0.152f) return 4;   // ev85 (1/6s)
    if (lux <= 0.185f) return 5;   // ev9 (1/8s)
    if (lux <= 0.22f) return 6;    // ev95 (1/10s)
    if (lux <= 0.275f) return 7;   // ev10 (1/15s)
    if (lux <= 0.345f) return 8;   // ev105 (1/20s)
    if (lux <= 0.468f) return 9;   // ev11 (1/30s)
    if (lux <= 0.58f) return 10;   // ev115 (1/45s)
    if (lux <= 0.802f) return 11;  // ev12 (1/60s)
    if (lux <= 1.115f) return 12;  // ev125 (1/90s)
    if (lux <= 1.6f) return 13;    // ev13 (1/125s)
    if (lux <= 2.35f) return 14;   // ev135 (1/180s)
    if (lux <= 3.4f) return 15;    // ev14 (1/250s)
    if (lux <= 4.2f) return 16;    // ev145 (1/360s)
    if (lux <= 10.0f) return 17;   // ev15 (1/500s)
    if (lux <= 100.0f) return 18;  // ev16 (1/1000s)
    return 19; // ev17 (1/2000s)
}

// 测光函数 (完全对应 Python light_meter())
float do_meter() {
    float lux_sum = 0.0f;
    int valid_count = 0;

    // Python: 连续读取 7 次取平均
    // tsl2561_read_lux() 内部已经处理了自动增益切换
    for (int i = 0; i < 7; i++) {
        float lux = tsl2561_read_lux(&lm);
        // 检查是否饱和（返回 -1.0f 表示饱和）
        if (lux >= 0.0f) {
            lux_sum += lux;
            valid_count++;
        }
        if (i < 6) sleep_ms(1);
    }

    if (valid_count == 0) {
        g_state.last_lux = 0.0f;
        g_state.auto_shutter_pos = 1;  // ev7 (1/2s) 最暗
        return 0.0f;
    }

    float lux_avg = lux_sum / valid_count;

    // Python: lux -= 0.4; lux *= 0.9
    float lux_adj = (lux_avg - 0.4f) * 0.9f;
    if (lux_adj < 0.0f) lux_adj = 0.0f;

    g_state.last_lux = lux_adj;

    // 计算 AUTO 档的快门速度
    g_state.auto_shutter_pos = calc_shutter_from_lux(lux_adj);
    if (g_state.auto_shutter_pos >= SHUTTER_SPEED_COUNT) {
        g_state.auto_shutter_pos = SHUTTER_SPEED_COUNT - 1;
    }

    return lux_adj;
}

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

    // 快门速度大字 (AUTO 档显示计算的快门，M 档显示手动设置的)
    uint8_t shutter_index = (g_state.menu == 0) ? g_state.auto_shutter_pos : g_state.m_pos;
    ssd1306_draw_str(&oled, 8, 18, shutter_speeds[shutter_index], &font5x8_font);

    // 显示 LUX 值 (右下角)
    char lux_str[16];
    if (g_state.menu == 0) {
        snprintf(lux_str, sizeof(lux_str), "L:%.2f", g_state.last_lux);
    } else {
        snprintf(lux_str, sizeof(lux_str), "L:---");
    }
    ssd1306_draw_str(&oled, 75, 20, lux_str, &font5x8_font);

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

// 初始化 TSL2561 测光表 (I2C0)
void tsl2561_init_system() {
    i2c_init(I2C_PORT_LM, 400 * 1000);
    gpio_set_function(I2C_PIN_LM_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_PIN_LM_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_PIN_LM_SDA);
    gpio_pull_up(I2C_PIN_LM_SCL);

    int ret = tsl2561_init(&lm, I2C_PORT_LM, TSL2561_I2C_ADDR);
    printf("TSL2561 @ 0x29: %s\r\n", ret == 0 ? "OK" : "FAIL");
    if (ret == 0) {
        uint8_t id = tsl2561_read_id(&lm);
        printf("TSL2561 ID: 0x%02X\r\n", id);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(3000);

    printf("\r\n=== SX-70 Mk2 启动 ===\r\n");

    // 初始化
    led_init();
    pcf8575_init_system();
    oled_init_system();
    tsl2561_init_system();

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

        // AUTO 模式下进行测光
        if (g_state.menu == 0) {
            do_meter();
        }

        show_frame();

        // 打印按键状态和测光数据
        read_3d_button_pins(btn);
        if (g_state.menu == 0) {
            printf("3D Button: up=%c down=%c mid=%c | Menu=%d Mode=%s | LUX=%.2f Shutter=%s\r\n",
                    btn[0], btn[1], btn[2], g_state.menu, g_state.cam_mode,
                    g_state.last_lux, shutter_speeds[g_state.auto_shutter_pos]);
        } else {
            printf("3D Button: up=%c down=%c mid=%c | Menu=%d Mode=%s\r\n",
                    btn[0], btn[1], btn[2], g_state.menu, g_state.cam_mode);
        }
        sleep_ms(100);
    }
}
