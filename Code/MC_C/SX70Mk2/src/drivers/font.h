/**
 * Copyright (c) 2025 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#ifndef FONTS_FONT_H
#define FONTS_FONT_H

#include "pico/stdlib.h"

typedef struct {
    const uint8_t *data;
    uint8_t width;
    uint8_t height;
    uint8_t first;
    uint8_t count;
} ssd1306_font_t;

#endif // FONTS_FONT_H
