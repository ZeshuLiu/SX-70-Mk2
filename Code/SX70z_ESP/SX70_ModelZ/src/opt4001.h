#pragma once

#include <esp_err.h>
#include <stdint.h>

esp_err_t opt4001_init(void);
esp_err_t opt4001_read_lux(float *lux);
