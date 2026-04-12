/**
 * Copyright (c) 2025 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#include <stdlib.h>
#include <string.h>
#include <hardware/i2c.h>
#include "ssd1306.h"
#include "lib/image.h"

static const uint8_t SET_CONTRAST = 0x81;
static const uint8_t SET_ENTIRE_ON = 0xA4;
static const uint8_t SET_NORM_INV = 0xA6;
static const uint8_t SET_DISP = 0xAE;
static const uint8_t SET_MEM_ADDR = 0x20;
static const uint8_t SET_COL_ADDR = 0x21;
static const uint8_t SET_PAGE_ADDR = 0x22;
static const uint8_t SET_DISP_START_LINE = 0x40;
static const uint8_t SET_SEG_REMAP = 0xA0;
static const uint8_t SET_MUX_RATIO = 0xA8;
static const uint8_t SET_COM_OUT_DIR = 0xC0;
static const uint8_t SET_DISP_OFFSET = 0xD3;
static const uint8_t SET_COM_PIN_CFG = 0xDA;
static const uint8_t SET_DISP_CLK_DIV = 0xD5;
static const uint8_t SET_PRECHARGE = 0xD9;
static const uint8_t SET_VCOM_DESEL = 0xDB;
static const uint8_t SET_CHARGE_PUMP = 0x8D;

static void write_command(ssd1306_t *dev, uint8_t cmd) {
  // Control byte 0x00 for commands
  uint8_t buffer[2] = {0x00, cmd};

  i2c_write_blocking(dev->i2c_inst, dev->i2c_addr, buffer, 2, false);
}

static void run_init_commands(ssd1306_t *dev) {
  // Init commands for the display based on the SSD1306 datasheet
  const uint8_t init_commands[] = {
      // Display off
      SET_DISP,
      // Timing and driving scheme
      SET_MUX_RATIO, (uint8_t)(dev->height - 1),
      SET_DISP_OFFSET, 0x00,
      SET_DISP_START_LINE,
      // Resolution and layout
      SET_SEG_REMAP | 0x01,
      SET_COM_OUT_DIR | 0x08,
      SET_COM_PIN_CFG, (dev->width > 2 * dev->height) ? 0x02 : 0x12,
      // Display
      SET_CONTRAST, 0xFF,
      SET_ENTIRE_ON,
      SET_NORM_INV,
      SET_DISP_CLK_DIV, 0x80,
      // Charge pump
      SET_CHARGE_PUMP, (dev->external_vcc ? 0x10 : 0x14),
      SET_PRECHARGE, (dev->external_vcc ? 0x22 : 0xF1),

      SET_VCOM_DESEL, 0x30,
      // Address setting
      SET_MEM_ADDR, 0x00,  // Horizontal
      // Display on
      SET_DISP | 0x01,
  };

  for (size_t i = 0; i < sizeof(init_commands); i++) {
    // TODO: Check return value in case of failure
    write_command(dev, init_commands[i]);
  }
}

static void draw_pixel(ssd1306_t *dev, uint16_t x, uint16_t y, bool color) {
  if (x < dev->width && y < dev->height) {
    // Shorthands for y / 8 and y % 8
    if (color) {
      dev->buff[x + dev->width * (y >> 3)] |= 0x01u << (y & 7);
    } else {
      dev->buff[x + dev->width * (y >> 3)] &= ~(0x01u << (y & 7));
    }
  }
}

static void fill_rect(ssd1306_t *dev, int16_t x_in, int16_t y_in,
                      uint16_t width, uint16_t height, bool color) {
  uint16_t x = x_in < 0 ? 0 : x_in;
  uint16_t y = y_in < 0 ? 0 : y_in;
  uint16_t x_end = x + width;
  uint16_t y_end = y + height;
  x_end = x_end > dev->width ? dev->width : x_end;
  y_end = y_end > dev->height ? dev->height : y_end;

  for (uint16_t i = x; i < x_end; ++i) {
    for (uint16_t j = y; j < y_end; ++j) {
      draw_pixel(dev, i, j, color);
    }
  }
}

static void draw_char(ssd1306_t *dev, uint16_t x, uint16_t y, const char *chr,
                      const ssd1306_font_t *font) {
  for (uint8_t i = 0; i < font->width; i++) {
    uint8_t line = font->data[(*chr - font->first) * font->width + i];
    for (uint8_t j = 0; j < font->height; j++) {
      draw_pixel(dev, x + i, y + j, (line & 0x01u));
      line >>= 1;
    }
  }
}

bool ssd1306_init(ssd1306_t *dev, uint16_t width, uint16_t height,
                  uint8_t i2c_addr, i2c_inst_t *i2c_inst, bool external_vcc) {
  dev->width = width;
  dev->height = height;
  dev->pages = height / 8;
  dev->i2c_addr = i2c_addr;
  dev->i2c_inst = i2c_inst;
  dev->external_vcc = external_vcc;
  dev->buff_size = width * dev->pages;

  // Allocate one extra byte for the control byte prefix used when writing
  if ((dev->buff = (uint8_t *) malloc(dev->buff_size + 1)) == NULL) {
    return false;
  }
  // Advance pointer so dev->buff points to the display data; dev->buff - 1 is the control byte
  (dev->buff)++;
  run_init_commands(dev);
  ssd1306_scroll_horiz_stop(dev);
  return true;
}

void ssd1306_deinit(ssd1306_t *dev) {
  if (dev->buff) {
    // Move pointer back to original for free
    free(dev->buff - 1);
    dev->buff = NULL;
  }
}

void ssd1306_power_off(ssd1306_t *dev) {
  write_command(dev, SET_DISP);
}

void ssd1306_power_on(ssd1306_t *dev) {
  write_command(dev, SET_DISP | 0x01);
}

void ssd1306_clear(ssd1306_t *dev) {
  memset(dev->buff, 0, dev->buff_size);
}

void ssd1306_invert(ssd1306_t *dev, uint8_t inv) {
  write_command(dev, SET_NORM_INV | (inv & 1));
}

void ssd1306_show(ssd1306_t *dev) {
  uint8_t data[] = {
    SET_COL_ADDR, 0x00, dev->width - 1,
    SET_PAGE_ADDR, 0x00, dev->pages - 1
  };
  for (size_t i = 0; i < sizeof(data); i++) {
    write_command(dev, data[i]);
  }
  // Control byte 0x40 for data
  *(dev->buff - 1) = 0x40;
  i2c_write_blocking(dev->i2c_inst, dev->i2c_addr, dev->buff - 1, dev->buff_size + 1, false);
}

void ssd1306_contrast(ssd1306_t *p, uint8_t val) {
  write_command(p, SET_CONTRAST);
  write_command(p, val);
}

void ssd1306_draw_pixel(ssd1306_t *dev, uint16_t x, uint16_t y) {
  draw_pixel(dev, x, y, 1);
}

void ssd1306_clear_pixel(ssd1306_t *dev, uint16_t x, uint16_t y) {
  draw_pixel(dev, x, y, 0);
}

void ssd1306_draw_line(ssd1306_t *dev, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  // Implements Bresenham's line algorithm
  // See: https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  int16_t D, dx, dy, step_x = 1, step_y = 1;

  dx = x2 - x1;
  dy = y2 - y1;
  if (dx < 0) {
      dx = -dx;
      step_x = -step_x;
  }

  if (dy < 0) {
      dy = -dy;
      step_y = -step_y;
  }

  draw_pixel(dev, x1, y1, 1);
  if (dy < dx) {
      D = dy * 2 - dx;
      while (x1 != x2) {
          x1 += step_x;
          if (D >= 0) {
              y1 += step_y;
              D -= 2 * dx;
          }
          D += 2 * dy;
          draw_pixel(dev, x1, y1, 1);
      }
  } else {
      D = dy - dx * 2;
      while (y1 != y2) {
          y1 += step_y;
          if (D <= 0) {
              x1 += step_x;
              D += 2*dy;
          }
          D -= 2*dx;
          draw_pixel(dev, x1, y1, 1);
      }
  }
}

void ssd1306_draw_rect(ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height) {
  for (int16_t i = x; i < x + (int16_t) width; i++) {
    draw_pixel(dev, i, y, 1);
    draw_pixel(dev, i, y + height - 1, 1);
  }
  for (int16_t i = y; i < y + (int16_t) height; i++) {
    draw_pixel(dev, x, i, 1);
    draw_pixel(dev, x + width - 1, i, 1);
  }
}

void ssd1306_draw_ellipse(ssd1306_t *dev, int16_t x_center, int16_t y_center,
                          uint16_t r_horiz, uint16_t r_vert) {
  // Implements the midpoint ellipse algorithm
  // See: https://en.wikipedia.org/wiki/Midpoint_circle_algorithm

  if (!r_horiz || !r_vert) {
    return;
  }
  // Bounding box check
  if ((x_center + (int16_t)r_horiz) < 0 || (x_center - (int16_t)r_horiz) >= dev->width ||
      (y_center + (int16_t)r_vert) < 0 || (y_center - (int16_t)r_vert) >= dev->height) {
    return;
  }

  int16_t x = 0;
  int16_t y = (int16_t) r_vert;

  float rx = (float) r_horiz;
  float ry = (float) r_vert;
  float rx2 = rx * rx;
  float ry2 = ry * ry;
  float two_rx2 = 2.0f * rx2;
  float two_ry2 = 2.0f * ry2;

  float dx = 0.0f;
  float dy = two_rx2 * (float) y;
  float d1 = ry2 - (rx2 * ry) + (0.25f * rx2);

  while (dx <= dy) {
    draw_pixel(dev, x_center + x, y_center + y, 1);
    draw_pixel(dev, x_center - x, y_center + y, 1);
    draw_pixel(dev, x_center + x, y_center - y, 1);
    draw_pixel(dev, x_center - x, y_center - y, 1);

    if (d1 < 0.0f) {
      x++;
      dx += two_ry2;
      d1 += dx + ry2;
    } else {
      x++;
      y--;
      dx += two_ry2;
      dy -= two_rx2;
      d1 += dx - dy + ry2;
    }
  }

  float d2 = (ry2 * (x + 0.5f) * (x + 0.5f)) +
             (rx2 * (y - 1.0f) * (y - 1.0f)) -
             (rx2 * ry2);

  while (y >= 0) {
    draw_pixel(dev, x_center + x, y_center + y, 1);
    draw_pixel(dev, x_center - x, y_center + y, 1);
    draw_pixel(dev, x_center + x, y_center - y, 1);
    draw_pixel(dev, x_center - x, y_center - y, 1);

    if (d2 > 0.0f) {
      y--;
      dy -= two_rx2;
      d2 += rx2 - dy;
    } else {
      y--;
      x++;
      dx += two_ry2;
      dy -= two_rx2;
      d2 += dx - dy + rx2;
    }
  }
}

void ssd1306_draw_circle(ssd1306_t *dev, int16_t x_center, int16_t y_center, uint16_t r) {
  ssd1306_draw_ellipse(dev, x_center, y_center, r, r);
}

void ssd1306_fill_rect(ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height) {
  fill_rect(dev, x, y, width, height, 1);
}

void ssd1306_clear_rect(ssd1306_t *dev, int16_t x, int16_t y, uint16_t width, uint16_t height) {
  fill_rect(dev, x, y, width, height, 0);
}

void ssd1306_draw_str(ssd1306_t *dev, int x, int y, const char *str, const ssd1306_font_t *font) {
  const uint8_t last = font->first + font->count;

  do {
    uint8_t ch = (uint8_t) *str;
    // Skip characters not in the font
    if (ch < font->first || ch >= last) {
        x += font->width;
        continue;
    }
    draw_char(dev, x, y, str, font);
    x += font->width;
  } while (*(++str));
}

void ssd1306_draw_image(ssd1306_t *dev, uint16_t x, uint16_t y, const ssd1306_image_t *image) {
  for (uint16_t j = 0; j < image->height; j++) {
    for (uint16_t i = 0; i < image->width; i++) {
      size_t byte_index = (i + j * image->width) / 8;
      uint8_t bit_index = 7 - (i % 8);
      bool pixel_on = (image->data[byte_index] >> bit_index) & 0x01u;
      draw_pixel(dev, x + i, y + j, pixel_on);
    }
  }
}

void ssd1306_scroll_horiz(ssd1306_t *dev, bool right, uint8_t start_page, uint8_t end_page, uint8_t speed) {
  ssd1306_scroll_horiz_stop(dev);
  write_command(dev, right ? 0x26 : 0x27);
  write_command(dev, 0x00);
  write_command(dev, start_page & 0x07);
  write_command(dev, speed & 0x00);
  write_command(dev, end_page & 0x07);
  write_command(dev, 0x00);
  write_command(dev, 0xFF);
  write_command(dev, 0x2F);
}

void ssd1306_scroll_horiz_stop(ssd1306_t *dev) {
  write_command(dev, 0x2E);
}

void ssd1306_scroll_row_vert(ssd1306_t *dev, bool down) {
  uint16_t width = dev->width;
  uint16_t pages = dev->height / 8;

  // Run through the columns and shift each column's bytes to given direction
  for (uint16_t col = 0; col < width; ++col) {
    uint8_t carry = 0;

    if (down) {
      for (uint16_t page = 0; page < pages; ++page) {
        uint16_t idx = page * width + col;
        uint8_t byte = dev->buff[idx];
        // Bit leaving the bottom
        uint8_t new_carry = (byte & 0x80u) ? 0x01u : 0u;
        dev->buff[idx] = (uint8_t)((byte << 1) | carry);
        carry = new_carry;
      }
      // Wrap the last bit to the top pixel
      dev->buff[col] = (dev->buff[col] & ~0x01u) | carry;
    } else {
      for (int page = (int)pages - 1; page >= 0; --page) {
        uint16_t idx = (uint16_t)page * width + col;
        uint8_t byte = dev->buff[idx];
        // Bit leaving the top
        uint8_t new_carry = (byte & 0x01u) ? 0x80u : 0u;
        dev->buff[idx] = (uint8_t)((byte >> 1) | carry);
        carry = new_carry;
      }
      uint16_t last_idx = (pages - 1) * width + col;
      // Wrap the last bit to the bottom pixel
      dev->buff[last_idx] = (dev->buff[last_idx] & ~0x80u) | carry;
    }
  }
}
