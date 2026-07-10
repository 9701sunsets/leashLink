#include "ll_tasks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buzzer_driver.h"
#include "motor_driver.h"
#include "safety_service.h"

/**
 * 执行器任务
 */
void task_actuator(void *arg)
{
    (void)arg;
    ll_safety_state_t last = LL_SAFETY_SAFE;
    while (true) {
        ll_safety_state_t state = safety_service_get_state();
        if (state != last) {
            if (state == LL_SAFETY_BURST_ALERT || state == LL_SAFETY_LOCKED) {
                buzzer_beep(120);
                motor_vibrate(180);
            } else if (state == LL_SAFETY_DISTANCE_ALERT) {
                buzzer_beep(80);
                motor_vibrate(80);
            }
            last = state;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

