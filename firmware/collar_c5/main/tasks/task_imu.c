#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "espnow_handle.h"
#include "imu_driver.h"
#include "motion_service.h"
#include "power_service.h"

static const char *TAG = "task_imu";

/**
 * IMU采样任务
 */
void task_imu(void *arg)
{
    (void)arg;
    uint8_t heartbeat_div = 0;
    uint8_t log_div = 0;
    collar_imu_status_t imu_status = imu_get_status();
    while (true) {
        uint16_t peak_mg = 0;
        int16_t temp_c_x10 = 0;
        imu_read_accel_peak(&peak_mg);
        imu_read_temp(&temp_c_x10);
        uint8_t confidence_pct = imu_status.bmi270_present ? 80 : 0;
        motion_service_update(peak_mg, confidence_pct);

        ll_collar_telemetry_t telemetry = {
            .motion_state = imu_status.bmi270_present ? motion_service_get_state() : LL_MOTION_UNKNOWN,
            .steps = 0,
            .accel_peak_mg = peak_mg,
            .confidence_pct = confidence_pct,
            .rssi_dbm = -60,
            .battery_pct = power_service_get_battery_pct(),
            .temp_c_x10 = temp_c_x10,
        };

        espnow_handle_send_telemetry(&telemetry);
        if (++heartbeat_div >= 5) {
            heartbeat_div = 0;
            espnow_handle_send_heartbeat((uint8_t)telemetry.motion_state, 0);
        }
        if (++log_div >= 5) {
            log_div = 0;
            ESP_LOGI(TAG,
                     "c5 data bmi=%d bmm=%d motion=%u accel_mg=%u temp_x10=%d bat=%u rssi=%d",
                     imu_status.bmi270_present,
                     imu_status.bmm350_present,
                     (unsigned)telemetry.motion_state,
                     telemetry.accel_peak_mg,
                     telemetry.temp_c_x10,
                     (unsigned)telemetry.battery_pct,
                     telemetry.rssi_dbm);
        }
        ESP_LOGD(TAG, "telemetry motion=%u accel=%u battery=%u", telemetry.motion_state, telemetry.accel_peak_mg, telemetry.battery_pct);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
