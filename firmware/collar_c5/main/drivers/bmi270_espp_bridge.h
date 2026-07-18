#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x_g;
    float y_g;
    float z_g;
    int16_t x_mg;
    int16_t y_mg;
    int16_t z_mg;
} collar_bmi270_accel_t;

esp_err_t collar_bmi270_espp_init(void);
bool collar_bmi270_espp_is_ready(void);
esp_err_t collar_bmi270_espp_read_accel(collar_bmi270_accel_t *out);

#ifdef __cplusplus
}
#endif
