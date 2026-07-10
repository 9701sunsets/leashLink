#include "ll_tasks.h"

#include "esp_log.h"
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
    while (true) {
        ll_tension_sample_t sample;
        esp_err_t err = tension_service_sample(&sample);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "sample failed: %s", esp_err_to_name(err));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

