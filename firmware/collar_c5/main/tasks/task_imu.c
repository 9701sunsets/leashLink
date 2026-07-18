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
    ESP_LOGI(TAG, "task_imu starting; initialize IMU now");
    esp_err_t imu_err = imu_init();
    if (imu_err != ESP_OK) {
        ESP_LOGW(TAG, "IMU init returned %s; telemetry will keep running with unknown motion",
                 esp_err_to_name(imu_err));
    }

    uint8_t heartbeat_div = 0;
    uint8_t log_div = 0;
    uint8_t rescan_div = 0;
    while (true) {
        uint16_t peak_mg = 0;
        int16_t temp_c_x10 = 0;
        imu_read_accel_peak(&peak_mg);
        imu_read_temp(&temp_c_x10);
        collar_imu_status_t imu_status = imu_get_status();
        if (!imu_status.bmi270_present && !imu_status.bmm350_present && ++rescan_div >= 25) {
            rescan_div = 0;
            ESP_LOGW(TAG, "IMU not detected; rescan shuttle board");
            imu_init();
            imu_status = imu_get_status();
        }
        uint8_t confidence_pct = (imu_status.bmi270_present && imu_status.config_loaded) ? 80 : 0;
        motion_service_update(peak_mg, confidence_pct);

        ll_collar_telemetry_t telemetry = {
            .motion_state = (imu_status.bmi270_present && imu_status.config_loaded) ? motion_service_get_state() : LL_MOTION_UNKNOWN,
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
                     "c5 data bmi=%d cfg=%d bmm=%d motion=%u accel_mg=%u xyz_mg=%d/%d/%d raw=%02x%02x/%02x%02x/%02x%02x status=0x%02x internal=0x%02x temp_x10=%d bat=%u rssi=%d",
                     imu_status.bmi270_present,
                     imu_status.config_loaded,
                     imu_status.bmm350_present,
                     (unsigned)telemetry.motion_state,
                     telemetry.accel_peak_mg,
                     imu_status.accel_x_mg,
                     imu_status.accel_y_mg,
                     imu_status.accel_z_mg,
                     imu_status.last_acc_raw[1],
                     imu_status.last_acc_raw[0],
                     imu_status.last_acc_raw[3],
                     imu_status.last_acc_raw[2],
                     imu_status.last_acc_raw[5],
                     imu_status.last_acc_raw[4],
                     imu_status.status_reg,
                     imu_status.internal_status_reg,
                     telemetry.temp_c_x10,
                     (unsigned)telemetry.battery_pct,
                     telemetry.rssi_dbm);
        }
        ESP_LOGD(TAG, "telemetry motion=%u accel=%u battery=%u", telemetry.motion_state, telemetry.accel_peak_mg, telemetry.battery_pct);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
