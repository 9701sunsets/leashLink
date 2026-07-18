#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    bool present;
    uint8_t i2c_addr;
    int int_level;
    uint8_t part_id;
} ll_heart_sensor_status_t;

typedef struct {
    uint32_t ir;
    uint32_t red;
    bool valid;
} ll_heart_raw_sample_t;

esp_err_t heart_sensor_init(void);
ll_heart_sensor_status_t heart_sensor_get_status(void);
esp_err_t heart_sensor_read_raw(ll_heart_raw_sample_t *out);
