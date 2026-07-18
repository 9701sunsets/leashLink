#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "leashlink_types.h"

typedef struct {
    bool bmi270_present;
    bool bmm350_present;
    uint8_t bmi270_addr;
    uint8_t bmm350_addr;
    uint8_t bmi270_chip_id;
    uint8_t bmm350_chip_id;
} collar_imu_status_t;

esp_err_t imu_init(void);
esp_err_t imu_read_accel_peak(uint16_t *peak_mg);
esp_err_t imu_read_temp(int16_t *temp_c_x10);
collar_imu_status_t imu_get_status(void);
