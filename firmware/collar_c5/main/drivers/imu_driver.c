#include "imu_driver.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

static const char *TAG = "imu";

/**
 * 初始化IMU传感器
 */
esp_err_t imu_init(void)
{
    ESP_LOGI(TAG, "IMU stub initialized");
    return ESP_OK;
}

/**
 * 读取加速度峰值
 * @param peak_mg 加速度峰值，单位为mg
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t imu_read_accel_peak(uint16_t *peak_mg)
{
    if (!peak_mg) {
        return ESP_ERR_INVALID_ARG;
    }
    *peak_mg = 0;
    return ESP_OK;
}

/**
 * 读取温度
 * @param temp_c_x10 温度值，单位为摄氏度*10
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t imu_read_temp(int16_t *temp_c_x10)
{
    if (!temp_c_x10) {
        return ESP_ERR_INVALID_ARG;
    }
    *temp_c_x10 = 0;
    return ESP_OK;
}
