#pragma once

#include <stdint.h>
#include "esp_err.h"

/**
 * HX711校准参数
 */
typedef struct {
    int32_t zero_offset;
    float scale_counts_per_n;
} hx711_calibration_t;

esp_err_t hx711_init(void);
esp_err_t hx711_read_raw(int32_t *raw);
esp_err_t hx711_set_calibration(hx711_calibration_t cal);
esp_err_t hx711_read_newtons(float *newtons);
esp_err_t hx711_tare(uint16_t samples);

