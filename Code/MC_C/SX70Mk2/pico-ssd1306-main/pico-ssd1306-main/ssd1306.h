/**
 * Copyright (c) 2025 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include "lib/font.h"
#include "lib/image.h"

#ifndef SSD1306_H
#define SSD1306_H

typedef struct {
  uint16_t width;
  uint16_t height;
  uint16_t pages;
  uint8_t i2c_addr;
  i2c_inst_t *i2c_inst;
  bool external_vcc;
  uint8_t *buff;
  size_t buff_size;
} ssd1306_t;

// Prepare the controller, allocate the frame buffer, and run the power-on sequence
bool ssd1306_init(ssd1306_t *dev, uint16_t width, uint16_t height,
                  uint8_t i2c_addr, i2c_inst_t *i2c_inst, bool external_vcc);

// Enter low-power standby mode
void ssd1306_power_off(ssd1306_t *dev);

// Exit standby and enable panel output
void ssd1306_power_on(ssd1306_t *dev);

// Clear the in-memory frame buffer
void ssd1306_clear(ssd1306_t *dev);

// Invert display colors if inv is non-zero
void ssd1306_invert(ssd1306_t *dev, uint8_t inv);

// Flush the frame buffer to the display over I2C
void ssd1306_show(ssd1306_t *dev);

// Set contrast (brightness) to a value between 0 and 255
void ssd1306_contrast(ssd1306_t *p, uint8_t val);

// Set a single pixel in the frame buffer
void ssd1306_draw_pixel(ssd1306_t *dev, uint16_t x, uint16_t y);

// Clear a single pixel in the frame buffer
void ssd1306_clear_pixel(ssd1306_t *dev, uint16_t x, uint16_t y);

// Draw a straight line
void ssd1306_draw_line(ssd1306_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// Outline an axis-aligned rectangle
void ssd1306_draw_rect(ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height);

// Outline an ellipse centered at the given coordinates (can be negative)
void ssd1306_draw_ellipse(ssd1306_t *dev, int16_t x_center, int16_t y_center, uint16_t r_horiz, uint16_t r_vert);

// Outline a circle at the given coordinates
void ssd1306_draw_circle(ssd1306_t *dev, int16_t x_center, int16_t y_center, uint16_t r);

// Fill an axis-aligned rectangle
void ssd1306_fill_rect(ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height);

// Clear an axis-aligned rectangle
void ssd1306_clear_rect(ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height);

// Render a null-terminated string using the supplied bitmap font
void ssd1306_draw_str(ssd1306_t *display, int x, int y, const char *text, const ssd1306_font_t *font);

// Copy a monochrome bitmap into the frame buffer
void ssd1306_draw_image(ssd1306_t *dev, uint16_t x, uint16_t y, const ssd1306_image_t *image);

// Start horizontal scroll effect across a page range
void ssd1306_scroll_horiz(ssd1306_t *dev, bool right, uint8_t start_page, uint8_t end_page, uint8_t speed);

// Halt any active horizontal scroll
void ssd1306_scroll_horiz_stop(ssd1306_t *dev);

// Scroll vertically by one position
void ssd1306_scroll_row_vert(ssd1306_t *dev, bool down);

#endif // SSD1306_H
