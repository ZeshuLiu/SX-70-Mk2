/**
 * Copyright (c) 2025 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "ssd1306.h"
#include <hardware/gpio.h>
#include <hardware/i2c.h>

// Using fonts
#include "lib/fonts/font5x8.h"
#include "lib/fonts/font6x8.h"
#include "lib/fonts/font8x8.h"

#include "tools/image_pico_board.h"

#define I2C_PORT i2c1
#define SCL_PIN 19
#define SDA_PIN 18

static ssd1306_t display;

static void draw_5_point_star(ssd1306_t *dev, uint16_t center_x, uint16_t center_y, float scale) {
  // Move the star's center to the origin (0, 0) with offsets
  uint16_t offset_x = 15;
  uint16_t offset_y = 16;
  // Vertices along the outer edge of an upright 5-point star inside a 29x30 grid
  uint16_t v[10][2] = {
    // Start from the tip of the top corner going clockwise
    {15, 0}, {18, 11}, {30, 11}, {21, 18}, {25, 29},
    {15, 22}, {5, 29}, {9, 18}, {0, 11}, {11, 11}
  };

  // Connect the adjacent vertices, finally joining the last one to the first
  for (int i = 0; i < 10; i++) {
    ssd1306_draw_line(
      dev,
      center_x + (scale * (int16_t) (v[i][0] - offset_x)),
      center_y + (scale * (int16_t) (v[i][1] - offset_y)),
      center_x + (scale * (int16_t) (v[(i + 1) % 10][0] - offset_x)),
      center_y + (scale * (int16_t) (v[(i + 1) % 10][1] - offset_y))
    );
  }
}

void init_display(uint8_t pin_sda, uint8_t pin_scl) {
  i2c_init(I2C_PORT, 400 * 1000);
  gpio_set_function(pin_sda, GPIO_FUNC_I2C);
  gpio_set_function(pin_scl, GPIO_FUNC_I2C);
  gpio_pull_up(pin_sda);
  gpio_pull_up(pin_scl);
  ssd1306_init(&display, 128, 64, 0x3C, I2C_PORT, 0);
}

void demo_write() {
  ssd1306_clear(&display);
  ssd1306_draw_str(&display, 5, 5, "SSD1306 Demo", &font8x8_font);
  ssd1306_draw_str(&display, 5, 22, "Pico C/C++ SDK", &font6x8_font);
  ssd1306_draw_str(&display, 5, 32, "Fonts: 8x8 6x8 5x8", &font5x8_font);
  ssd1306_draw_str(&display, 5, 52, "github.com/tapiocode", &font5x8_font);
  ssd1306_show(&display);
}

void demo_contrast() {
  ssd1306_clear(&display);
  ssd1306_fill_rect(&display, 0, 0, display.width, display.height);
  ssd1306_show(&display);
  int16_t step = 32;
  int16_t i = 0;
  int16_t cycles = (255 / step) * 4;

  while (cycles--) {
    ssd1306_contrast(&display, (uint8_t) i);
    ssd1306_clear_rect(&display, 8, 10, 112, 16);
    char txt[20];
    sprintf(txt, "Contrast: %d", i);
    ssd1306_draw_str(&display, 12, 14, txt, &font8x8_font);
    ssd1306_show(&display);
    sleep_ms(100);
    // Clamp i between 0 and 255, reverse direction at limits
    i += step;
    i = i > 255 ? 255 : i < 0 ? 0 : i;
    if (i == 255 && step > 0 || i == 0 && step < 0) {
      step = -step; 
    }
  }
  ssd1306_contrast(&display, 255);
}

void demo_invert() {
  ssd1306_clear(&display);
  ssd1306_draw_str(&display, 10, 25, "Inverting...", &font8x8_font);
  ssd1306_show(&display);
  sleep_ms(500);
  ssd1306_invert(&display, 1);
  ssd1306_show(&display);
  sleep_ms(500);
  ssd1306_invert(&display, 0);
  ssd1306_show(&display);
  sleep_ms(500);
}

void demo_pixel_drawing() {
  uint16_t half = display.width / 2;
  ssd1306_clear(&display);
  ssd1306_fill_rect(&display, 0, 0, half, display.height);
  for (uint16_t i = 0; i < 100; i++) {
    uint16_t x0 = rand() % half + half;
    uint16_t y0 = rand() % display.height;
    uint16_t x1 = rand() % half;
    uint16_t y1 = rand() % display.height;
    ssd1306_draw_pixel(&display, x0, y0);
    ssd1306_clear_pixel(&display, x1, y1);
    ssd1306_show(&display);
  }
}

void demo_lines() {
  ssd1306_clear(&display);
  // 1. A box around the edges drawn in order from top-left corner clockwise
  uint16_t tl[2] = {0, 0};
  uint16_t tr[2] = {display.width - 1, 0};
  uint16_t bl[2] = {0, display.height - 1};
  uint16_t br[2] = {display.width - 1, display.height - 1};
  ssd1306_draw_line(&display, tl[0], tl[1], tr[0], tr[1]);
  ssd1306_draw_line(&display, tr[0], tr[1], br[0], br[1]);
  ssd1306_draw_line(&display, br[0], br[1], bl[0], bl[1]);
  ssd1306_draw_line(&display, bl[0], bl[1], tl[0], tl[1]);

  // 2. Diagonals from corner to corner
  ssd1306_draw_line(&display, 0, 0, display.width - 1, display.height - 1);
  ssd1306_draw_line(&display, 0, display.height - 1, display.width - 1, 0);
  ssd1306_show(&display);
}

void demo_scaling_star() {
  uint16_t star_x = display.width / 2 - 20;
  uint16_t star_y = display.height / 2 - 15;
  size_t cycles = 5;
  float scale = 0.6f;
  float step = 0.1f;

  while (cycles > 0) {
    ssd1306_clear(&display);
    draw_5_point_star(&display,
      star_x + (int16_t)(scale * 10),
      star_y + (int16_t)(scale * 15),
      scale);
    ssd1306_show(&display);
    // Reverse direction if limits are reached
    if (scale > 3.0f && step > 0.0f || scale < 0.8f && step < 0.0f) {
      step = -step;
      cycles -= 1;
    }
    scale += step;
  }
}

void demo_scrolling_stars() {
  ssd1306_clear(&display);
  // Draw the star field in a staggered grid pattern
  for (uint16_t y = 10; y < display.height; y += 16) {
    for (uint16_t x = (y / 16) % 2 == 0 ? 10 : 19; x < display.width; x += 25) {
      draw_5_point_star(&display, x, y, 0.45f);
    }
  }
  ssd1306_show(&display);
  // Scroll up
  for (int i = 0; i < display.height; i++) {
    ssd1306_scroll_row_vert(&display, true);
    ssd1306_show(&display);
  }
  // Scroll right
  ssd1306_scroll_horiz(&display, true, 0, display.pages - 1, 0);
  sleep_ms(1500);
  ssd1306_scroll_horiz_stop(&display);
  // Scroll down
  for (int i = 0; i < display.height; i++) {
    ssd1306_scroll_row_vert(&display, false);
    ssd1306_show(&display);
  }
  // Scroll left
  ssd1306_scroll_horiz(&display, false, 0, display.pages - 1, 0);
  sleep_ms(1500);
  ssd1306_scroll_horiz_stop(&display);
}

void demo_rectangles() {
  uint16_t x_cent = display.width / 2;
  uint16_t y_cent = display.height / 2;
  int cycles = 10, x = 0;

  while (cycles--) {
    for (uint16_t i = 0; i < 15; i++) {
      ssd1306_clear(&display);
      for (uint16_t r = i; r < (display.width * 2); r += 15) {
        ssd1306_draw_rect(&display,
          (x_cent - r / 2) + x - 100,
          (y_cent - r / 2),
          r,
          r);
      }
      ssd1306_show(&display);
      x += 1;
    }
  }
}

void demo_ellipses() {
  int16_t x_cent = display.width / 2;
  int16_t y_cent = display.height / 2;
  uint16_t cycles = 5;

  while (cycles--) {
    for (uint16_t i = 0; i < 20; i++) {
      ssd1306_clear(&display);
      uint16_t wt = i;
      uint16_t ht = i / 4;

      // Draw two opposing slightly offset ellipse patterns
      for (uint16_t r = 0; r < (display.width * 2); r += 20) {
        ssd1306_draw_ellipse(&display, -20, y_cent - 10, wt, ht);
        ssd1306_draw_ellipse(&display, display.width + 20, y_cent + 10, wt, ht);
        wt += 20;
        ht += 5;
      }

      for (uint16_t r = i; r <= display.width; r += 20) {
        ssd1306_draw_circle(&display, x_cent, y_cent, r);
      }
      ssd1306_show(&display);
    }
  }
}

void demo_fills() {
  int16_t x_cent = display.width / 4;
  int16_t y_cent = display.height / 2;
  uint16_t cycles = 5;

  while (cycles--) {
    for (uint16_t i = 0; i < 20; i++) {
      ssd1306_clear(&display);
      // Draw hollowed out rectangles from largest to smallest around the centerpoint
      for (int16_t r = display.height + i; r > 0; r -= 20) {
        ssd1306_fill_rect(&display,
          x_cent - r / 2,
          y_cent - r / 2,
          r * 1.75f,
          r);
        if ((r - 10) > 0) {
          ssd1306_clear_rect(&display,
            x_cent - r / 2 + 5,
            y_cent - r / 2 + 5,
            (r - 10) * 1.75f,
            r - 10);
        }
      }
      ssd1306_show(&display);
    }
  }
}

void demo_power_onoff() {
  ssd1306_clear(&display);
  ssd1306_draw_str(&display, 5, 25, "Powering off...", &font8x8_font);
  ssd1306_show(&display);
  sleep_ms(1000);
  ssd1306_power_off(&display);
  sleep_ms(1000);
  ssd1306_power_on(&display);
  ssd1306_draw_str(&display, 5, 45, "Back on", &font8x8_font);
  ssd1306_show(&display);
  sleep_ms(1000);
}

void demo_scroll_oversize_image() {
  for (uint16_t i = 0; i < image_pico_board.height - display.height; i += 1) {
    ssd1306_clear(&display);
    ssd1306_draw_image(&display, 0, -i, &image_pico_board);
    ssd1306_show(&display);
  }
}

int main() {
  stdio_init_all();
  init_display(SDA_PIN, SCL_PIN);
  
  // Loop the demo reel endlessly
  while (1) {
    demo_write();
    sleep_ms(2000);
    demo_contrast();
    sleep_ms(750);
    demo_invert();
    sleep_ms(750);
    
    demo_pixel_drawing();
    demo_scaling_star();
    demo_scrolling_stars();
    sleep_ms(750);

    demo_scroll_oversize_image();
    sleep_ms(750);

    demo_lines();
    sleep_ms(750);
    demo_rectangles();
    demo_ellipses();
    demo_fills();
    demo_power_onoff();
  }
}
