#include "ll_tasks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "espnow_handle.h"
#include "oled_driver.h"
#include "power.h"
#include "safety_service.h"
#include "tension_service.h"

void task_ui(void *arg)
{
    (void)arg;
    while (true) {
        ll_tension_sample_t tension = tension_service_get_latest();
        ll_collar_telemetry_t collar = espnow_handle_get_collar();
        oled_show_status(&tension, &collar, safety_service_get_state(), power_get_battery_pct());
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

