#include "led.h"

static led_state_t g_led_state = {LED_OFF, LED_OFF};

void led_init(void) {
    // 初始化黄灯
    gpio_init(LED_Y_PIN);
    gpio_set_dir(LED_Y_PIN, GPIO_OUT);
    gpio_put(LED_Y_PIN, LED_OFF);

    // 初始化蓝灯
    gpio_init(LED_B_PIN);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
    gpio_put(LED_B_PIN, LED_OFF);

    g_led_state.led_y_state = LED_OFF;
    g_led_state.led_b_state = LED_OFF;
}

void led_close(void) {
    gpio_put(LED_Y_PIN, LED_OFF);
    gpio_put(LED_B_PIN, LED_OFF);
    g_led_state.led_y_state = LED_OFF;
    g_led_state.led_b_state = LED_OFF;
}

void led_iso(uint8_t iso_600) {
    if (iso_600) {
        // ISO 600: 点亮黄灯
        gpio_put(LED_Y_PIN, LED_ON);
        gpio_put(LED_B_PIN, LED_OFF);
        g_led_state.led_y_state = LED_ON;
        g_led_state.led_b_state = LED_OFF;
    } else {
        // 其他 ISO: 点亮蓝灯
        gpio_put(LED_Y_PIN, LED_OFF);
        gpio_put(LED_B_PIN, LED_ON);
        g_led_state.led_y_state = LED_OFF;
        g_led_state.led_b_state = LED_ON;
    }
}

void led_y_set(uint8_t state) {
    gpio_put(LED_Y_PIN, state);
    g_led_state.led_y_state = state;
}

void led_b_set(uint8_t state) {
    gpio_put(LED_B_PIN, state);
    g_led_state.led_b_state = state;
}

led_state_t led_get_state(void) {
    return g_led_state;
}
