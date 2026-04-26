#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"
#include "drivers/pcf8575.h"
#include "drivers/ssd1306.h"
#include "drivers/font.h"
#include "drivers/fonts/font5x8.h"
#include "drivers/led.h"
#include "drivers/tsl2561.h"
#include "pins.h"
#include "CONF.h"
#include "metering.h"
#include "shutter.h"

// 设备实例
static pcf8575_t pcf;
static ssd1306_t oled;
tsl2561_t lm;  // 测光表 (暴露给 metering.c)
uint8_t s1f = 0;
uint8_t s1t = 0;

// 相机状态 - 对应 Python Button3D 类
typedef struct {
    uint8_t menu;           // 菜单层级：0=AUTO, 1=BULB, 2=TIME, 3=MANUAL, 10=自拍
    uint8_t m_pos;          // M 档快门速度索引
    uint8_t self_timer_ind; // 自拍定时索引
    char cam_mode[8];       // 当前模式字符串 (最长 "1/2000" + \0 = 7 字节)
    float last_lux;         // 上次测光值 (浮点数)
    uint8_t auto_shutter_pos; // AUTO 档计算的快门位置
    char shut_mode;         // 快门模式：'0'=闪光，'1'=正常，'B'=B 门，'T'=T 门
    uint8_t shutter_speed;  // 当前快门速度索引
} camera_state_t;

static camera_state_t g_state = {0, 0, 0, "", 0, 0, '1', 1};

// 对焦状态 (对应 Python if_focused)
static uint8_t if_focused = 0;

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

// S1F 半按快门对焦检测
static uint8_t s1f_last = 1;  // 上次 S1F 状态 (1=未按下，0=按下)

// 闪光灯检测状态 (对应 Python flash_connected)
// S2 引脚：PULL_UP，闪光灯连接时接地 (0)，无闪光灯时上拉 (1)
static uint8_t flash_connected = 0;

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

void debounce_read_s1pin(void){
    bool s1f_t = gpio_get(S1F_PIN);
    bool s1t_t = gpio_get(S1T_PIN);

    if (s1f_t == 0){
        s1f = (s1f == 0)? s1f:s1f-1;
    }
    else{
        s1f = S1_DEBOUNCE_COUNT;
    }

    if (s1t_t == 0){
        s1t = (s1t == 0)? s1t:s1t-1;
    }
    else{
        s1t = S1_DEBOUNCE_COUNT;
    }
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
        snprintf(g_state.cam_mode, sizeof(g_state.cam_mode), "%s", get_shutter_speed(g_state.m_pos));
    } else if (g_state.menu == 10) {
        snprintf(g_state.cam_mode, sizeof(g_state.cam_mode), "---");
    }
}

// 对焦函数 (对应 Python Focus)
// 返回值：shut_mode (快门模式), shutter_speed (快门速度)
void do_focus(char *shut_mode, uint8_t *shutter_speed) {
    // 设置对焦标志
    if_focused = 1;
    // 设置 S1F 反馈引脚为高电平 (对应 Python S1F_FBW.value(1))
    gpio_put(S1F_FBW_PIN, 1);

    // 根据闪光灯状态和对焦模式决定行为
    if (flash_connected) {
        // 有闪光灯：闪光灯模式 (对应 Python Focus_Flash_work)
        *shut_mode = '0';  // 闪光模式

        // A 档或 M 档：根据情况限制快门速度
        if (g_state.menu == 0) {
            // A 档：固定 ev11
            *shutter_speed = 11;
        } else if (g_state.menu == 3) {
            // M 档：如果比 ev11 快（索引>11），则限制为 ev11；否则保持用户设置
            if (g_state.m_pos > 11) {
                *shutter_speed = 11;
            } else {
                *shutter_speed = g_state.m_pos;
            }
        } else {
            // B/T 档等其他模式
            *shutter_speed = 11;
        }
        printf("Flash connected! shutter=%s\r\n", get_shutter_speed(*shutter_speed));
    } else if (g_state.menu == 0) {
        // A 档 (Auto) - 自动测光决定快门速度
        *shut_mode = '1';  // 正常模式
        do_meter(&g_state.last_lux, &g_state.auto_shutter_pos);
        *shutter_speed = g_state.auto_shutter_pos;
    } else if (g_state.menu == 1) {
        // B 档 (Bulb)
        *shut_mode = 'B';
        *shutter_speed = 1;  // ev7 (1/2s)
    } else if (g_state.menu == 2) {
        // T 档 (Time)
        *shut_mode = 'T';
        *shutter_speed = 1;  // ev7 (1/2s)
    } else if (g_state.menu == 3) {
        // M 档 (Manual) - 使用手动设置的快门速度
        *shut_mode = '1';  // 正常模式
        *shutter_speed = g_state.m_pos;
        // M 档也进行测光（但不使用结果）
        do_meter(&g_state.last_lux, &g_state.auto_shutter_pos);
    } else {
        // 自拍等其他模式
        *shut_mode = '1';
        *shutter_speed = 1;
    }

    printf("Focus: mode=%c, shutter=%s\r\n", *shut_mode, get_shutter_speed(*shutter_speed));
}

// 松开对焦 (对应 Python 中松开 S1F 的处理)
void do_focus_release() {
    if_focused = 0;
    // 设置 S1F 反馈引脚为低电平 (对应 Python S1F_FBW.value(0))
    gpio_put(S1F_FBW_PIN, 0);
    printf("Focus Release\r\n");
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

    // 闪光灯指示 (对应 Python: self.display.text('FLASH', 86, 2, 0))
    if (flash_connected) {
        ssd1306_draw_str(&oled, 86, 2, "FLASH", &font5x8_font);
    } else {
        ssd1306_draw_str(&oled, 90, 2, "OFF", &font5x8_font);
    }

    // 快门速度大字 (AUTO 档显示计算的快门，M 档显示手动设置的)
    uint8_t shutter_index = (g_state.menu == 0) ? g_state.auto_shutter_pos : g_state.m_pos;
    // 根据快门速度字符串长度调整 x 坐标，确保短速度值 (如 1/2000) 居中显示
    int shutter_x = 8;
    if (shutter_index >= 17) {  // 1/500 及更快
        shutter_x = 4;
    } else if (shutter_index >= 14) {  // 1/250 ~ 1/360
        shutter_x = 6;
    }
    ssd1306_draw_str(&oled, shutter_x, 18, get_shutter_speed(shutter_index), &font5x8_font);

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

void check_body_parts() {
    char btn[4];
    uint8_t s3_state = gpio_get(S3_PIN);
    // uint8_t s5_state = gpio_get(S5_PIN);
    DEBUG_PRINTF("S3=%d\r\n", s3_state);

    if (s3_state == 0) {
        ssd1306_clear(&oled);
        ssd1306_draw_str(&oled, 10, 8, "ERROR 01", &font5x8_font);
        ssd1306_draw_str(&oled, 5, 20, "See Manual", &font5x8_font);
        ssd1306_show(&oled);
        DEBUG_PRINTF("[WARN] S3 条件满足 (S3!=0)\r\n");

        // 阻塞直到 button3d 任意按钮按下
        while (1) {
            read_3d_button_pins(btn);
            if (btn[0] == '0' || btn[1] == '0' || btn[2] == '0') {
                break;
            }
            sleep_ms(50);
        }
        sleep_ms(200); // 释放去抖
    }
}

int main() {
    stdio_init_all();
    sleep_ms(300);

    printf("\r\n=== SX-70 Mk2 启动 ===\r\n");
    printf("clk_sys = %u Hz\n", clock_get_hz(clk_sys));

    // 初始化输入引脚
    gpio_init(S1F_PIN);
    gpio_set_dir(S1F_PIN, GPIO_IN);

    gpio_init(S1T_PIN);
    gpio_set_dir(S1T_PIN, GPIO_IN);

    gpio_init(S2_PIN);
    gpio_set_dir(S2_PIN, GPIO_IN);
    gpio_pull_up(S2_PIN);  // 闪光灯检测：连接时接地 (0)，无闪光灯时上拉 (1)

    gpio_init(S3_PIN);
    gpio_set_dir(S3_PIN, GPIO_IN);
    gpio_pull_up(S3_PIN);

    gpio_init(S5_PIN);
    gpio_set_dir(S5_PIN, GPIO_IN);
    gpio_pull_up(S5_PIN);

    // 初始化输出引脚
    gpio_init(MOTOR_PIN);
    gpio_set_dir(MOTOR_PIN, GPIO_OUT);
    gpio_put(MOTOR_PIN, 0);

    gpio_init(FF_PIN);
    gpio_set_dir(FF_PIN, GPIO_OUT);
    gpio_put(FF_PIN, 0);

    // S1F 反馈引脚 (对应 Python S1F_FBW)
    gpio_init(S1F_FBW_PIN);
    gpio_set_dir(S1F_FBW_PIN, GPIO_OUT);
    gpio_put(S1F_FBW_PIN, 0);  // 初始为低电平

    // 初始化驱动
    shutter_init();  // 快门 PWM 初始化
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
    sleep_ms(100);

    button3d_handler();
    // 打印按键状态
    char btn[4];
    read_3d_button_pins(btn);
    if (btn[0]=='0'|| btn[1]=='0' || btn[2]=='0')
    {
        enter_bootloader();
    }

    // 检查 S3 和 S5 开关状态
    check_body_parts();

// 主循环 (对应 Python Cam_Operation)
    while (true) {
        button3d_handler();

        // 检测闪光灯连接状态 (对应 Python: self.flash_connected = not self.s2.value())
        // S2 上拉：无闪光灯=1，有闪光灯 (接地)=0
        flash_connected = !gpio_get(S2_PIN);

        // 读取 S1F (半按快门) 和 S1T (全按快门) 状态
        debounce_read_s1pin();
        bool s1f_pressed = (s1f > 0);
        bool s1t_pressed = (s1t > 0);

        // 半按快门对焦 (对应 Python: if foc == self.Red_Button_Pressed and self.if_focused == False)
        if (s1f_pressed == 1 && if_focused == 0) {
            if (g_state.menu == 10) {
                printf("不在拍摄模式\r\n");
            } else {
                do_focus(&g_state.shut_mode, &g_state.shutter_speed);
            }
        }

        // 松开半按快门 (对应 Python: if foc != self.Red_Button_Pressed and self.if_focused == True)
        if (s1f_pressed != 1 && if_focused == 1) {
            do_focus_release();
        }

        // AUTO 模式 并且 处于对焦状态下 进行测光
        if (g_state.menu == 0 && if_focused == 1) {
            do_meter(&g_state.last_lux, &g_state.auto_shutter_pos);
        }

        // 全按快门拍摄 (对应 Python: if tak == self.Red_Button_Pressed)
        if (s1t_pressed == 1) {
            // gpio_put(LED_Y_PIN, 0);  // 强制高电平
            // gpio_put(LED_B_PIN, 1);
            led_close();
            if (g_state.menu == 10) {
                printf("不在拍摄模式\r\n");
            } else {
                // 确保已对焦
                if (if_focused == 0) {
                    do_focus(&g_state.shut_mode, &g_state.shutter_speed);
                }

                // 获取快门时间 (0.1ms)
                uint16_t shutter_delay = get_shutter_time_x10(g_state.shutter_speed);

                // 执行曝光控制
                shutter_expose(shutter_delay, g_state.shut_mode);
            }

            // 等待全按快门释放，防止连拍
            while (gpio_debounce_defaultHigh(S1T_PIN)) {
                sleep_ms(10);
            }
        }

        show_frame();

        // 打印按键状态和测光数据
        read_3d_button_pins(btn);
        if (g_state.menu == 0) {
            printf("S1F=%d Focused=%d | Menu=%d Mode=%s | LUX=%.2f Shutter=%s\r\n",
                    s1f_pressed, if_focused, g_state.menu, g_state.cam_mode,
                    g_state.last_lux, get_shutter_speed(g_state.auto_shutter_pos));
        } else {
            printf("S1F=%d Focused=%d | Menu=%d Mode=%s\r\n",
                    s1f_pressed, if_focused, g_state.menu, g_state.cam_mode);
        }
        // sleep_ms(10);
    }
}
