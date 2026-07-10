#include "ll_tasks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cloud_service.h"
#include "espnow_handle.h"
#include "leash_control.h"
#include "power.h"
#include "tension_service.h"

void task_cloud(void *arg)
{
    (void)arg;
    while (true) {
        ll_tension_sample_t tension = tension_service_get_latest();
        ll_collar_telemetry_t collar = espnow_handle_get_collar();
        cloud_service_publish_telemetry(&tension, &collar, leash_control_get_state(), power_get_battery_pct());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

