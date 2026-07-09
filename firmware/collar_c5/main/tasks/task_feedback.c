#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "feedback_service.h"
#include "motion_service.h"
#include "power_service.h"

static const char *TAG = "task_feedback";

void task_feedback(void *arg)
{
    (void)arg;
    while (true) {
        if (motion_service_get_state() == LL_MOTION_BURST) {
            ll_control_cmd_t cmd = {
                .cmd_id = 1,
                .cmd_type = LL_FEEDBACK_DANGER,
                .duration_ms = 300,
                .param_a = 0,
                .param_b = 0,
            };
            feedback_service_handle_cmd(&cmd);
        }
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
