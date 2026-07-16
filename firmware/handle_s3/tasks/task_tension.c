#include "ll_tasks.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tension_service.h"

static const char *TAG = "task_tension";

/**
 * 张力采样任务
 */
void task_tension(void *arg)
{
    (void)arg;
    int64_t last_warn_ms = 0;
    while (true) {
        ll_tension_sample_t sample;
        esp_err_t err = tension_service_sample(&sample);
        if (err != ESP_OK) {
            int64_t now_ms = esp_timer_get_time() / 1000;
            if (now_ms - last_warn_ms >= 1000) {
                ESP_LOGW(TAG, "sample failed: %s", esp_err_to_name(err));
                last_warn_ms = now_ms;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
