#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    int raw;
    int lux_est;
    bool digital_dark;
} ll_light_sample_t;

esp_err_t light_sensor_init(void);
esp_err_t light_sensor_read(ll_light_sample_t *out);
