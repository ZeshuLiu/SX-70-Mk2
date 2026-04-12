/**
 * Copyright (c) 2025 tapiocode
 * https://github.com/tapiocode
 * MIT License
 */

#ifndef TOOLS_IMAGE_H
#define TOOLS_IMAGE_H

#include "pico/stdlib.h"

typedef struct {
    uint16_t width;
    uint16_t height;
    size_t length;
    const uint8_t *data;
} ssd1306_image_t;

#endif // TOOLS_IMAGE_H
