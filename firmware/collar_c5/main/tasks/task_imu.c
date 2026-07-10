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
    while (true) {
        uint16_t peak_mg = 0;
        int16_t temp_c_x10 = 0;
        imu_read_accel_peak(&peak_mg);
        imu_read_temp(&temp_c_x10);
        motion_service_update(peak_mg, 80);

        ll_collar_telemetry_t telemetry = {
            .motion_state = motion_service_get_state(),
            .steps = 0,
            .accel_peak_mg = peak_mg,
            .confidence_pct = 80,
            .rssi_dbm = -60,
            .battery_pct = power_service_get_battery_pct(),
            .temp_c_x10 = temp_c_x10,
        };

        espnow_handle_send_telemetry(&telemetry);
        ESP_LOGD(TAG, "telemetry motion=%u accel=%u battery=%u", telemetry.motion_state, telemetry.accel_peak_mg, telemetry.battery_pct);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
