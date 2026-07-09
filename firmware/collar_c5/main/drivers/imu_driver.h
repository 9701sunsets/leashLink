#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t imu_init(void);
esp_err_t imu_read_accel_peak(uint16_t *peak_mg);
esp_err_t imu_read_temp(int16_t *temp_c_x10);
