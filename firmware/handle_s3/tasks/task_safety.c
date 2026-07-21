#include "ll_tasks.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cloud_service.h"
#include "distance_service.h"
#include "espnow_handle.h"
#include "gps_driver.h"
#include "leash_control.h"
#include "safety_service.h"
#include "tension_service.h"

static const char *TAG = "task_safety";

/**
 * 安全评估任务
 */
void task_safety(void *arg)
{
    (void)arg;
    ll_safety_state_t last_actuated_state = LL_SAFETY_SAFE;
    while (true) {
        ll_tension_sample_t tension = tension_service_get_latest();
        ll_collar_telemetry_t collar = espnow_handle_get_collar();
        ll_gps_fix_t gps = gps_get_latest();
        bool link_ok = espnow_handle_link_ok();

        ll_safety_input_t input = {
            .tension_n = tension.tension_n,
            .tension_peak_n = tension.tension_peak_n,
            .collar_accel_peak_g = collar.accel_peak_g,
            .motion_state = collar.motion_state,
            .distance_est_m = link_ok ? distance_service_estimate_from_rssi(collar.rssi_dbm) : 0.0f,
            .gps_fence_breach = distance_service_is_fence_breach(&gps),
            .link_ok = link_ok,
            .ts_ms = esp_timer_get_time() / 1000,
        };

        ll_safety_event_t event;
        ESP_ERROR_CHECK(safety_service_eval(&input, &event));

        ll_safety_state_t state = safety_service_get_state();
        if (state != last_actuated_state) {
            if (state == LL_SAFETY_BURST_ALERT || state == LL_SAFETY_LOCKED) {
                leash_control_lock(1000);
            } else if (state == LL_SAFETY_SAFE || state == LL_SAFETY_RECOVERY) {
                leash_control_unlock();
            }
            last_actuated_state = state;
        }

        if (event.type == LL_EVENT_BURST_PULL) {
            ESP_LOGW(TAG, "burst event tension=%.1f accel=%.2f", event.tension_peak_n, event.accel_peak_g);
            espnow_handle_send_feedback(3, 1000);
            cloud_service_publish_event(&event);
        } else if (event.type == LL_EVENT_DISTANCE_WARNING || event.type == LL_EVENT_FENCE_BREACH) {
            espnow_handle_send_feedback(4, 5000);
            cloud_service_publish_event(&event);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
