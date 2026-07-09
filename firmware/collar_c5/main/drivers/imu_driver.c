#include "imu_driver.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

static const char *TAG = "imu";

esp_err_t imu_init(void)
{
    ESP_LOGI(TAG, "IMU stub initialized");
    return ESP_OK;
}

esp_err_t imu_read_accel_peak(uint16_t *peak_mg)
{
    if (!peak_mg) {
        return ESP_ERR_INVALID_ARG;
    }
    *peak_mg = 0;
    return ESP_OK;
}

esp_err_t imu_read_temp(int16_t *temp_c_x10)
{
    if (!temp_c_x10) {
        return ESP_ERR_INVALID_ARG;
    }
    *temp_c_x10 = 0;
    return ESP_OK;
}
