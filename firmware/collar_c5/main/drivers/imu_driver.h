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
    uint8_t shuttle_g1_level;
    uint8_t shuttle_g2_level;
    uint8_t shuttle_sdo_level;
    int16_t accel_x_mg;
    int16_t accel_y_mg;
    int16_t accel_z_mg;
    bool bmi270_spi;
    uint8_t status_reg;
    uint8_t internal_status_reg;
    uint8_t pwr_ctrl_reg;
    uint8_t acc_conf_reg;
    uint8_t acc_range_reg;
    uint8_t last_acc_raw[6];
    bool config_loaded;
} collar_imu_status_t;

esp_err_t imu_init(void);
esp_err_t imu_read_accel_peak(uint16_t *peak_mg);
esp_err_t imu_read_temp(int16_t *temp_c_x10);
collar_imu_status_t imu_get_status(void);
