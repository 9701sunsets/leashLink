#include "ll_tasks.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buzzer_driver.h"
#include "button_driver.h"
#include "esp_log.h"
#include "espnow_handle.h"
#include "leash_control.h"
#include "motor_driver.h"
#include "safety_service.h"

static const char *TAG = "task_actuator";

/**
 * 执行器任务
 */
void task_actuator(void *arg)
{
    (void)arg;
    ll_safety_state_t last = LL_SAFETY_SAFE;
    ll_button_state_t prev_buttons = {0};
    while (true) {
        ll_button_state_t buttons = button_driver_get_state();
        if (buttons.button1_pressed && !prev_buttons.button1_pressed) {
            ESP_LOGI(TAG, "B1 manual unlock level=%d", buttons.button1_level);
            leash_control_unlock();
            buzzer_beep(40);
        }
        if (buttons.button2_pressed && !prev_buttons.button2_pressed) {
            ESP_LOGI(TAG, "B2 manual lock level=%d", buttons.button2_level);
            leash_control_lock(0);
            buzzer_beep(120);
        }
        if (buttons.button3_pressed && !prev_buttons.button3_pressed) {
            ESP_LOGI(TAG, "B3 collar warning level=%d", buttons.button3_level);
            espnow_handle_send_feedback(LL_FEEDBACK_WARNING, 300);
            buzzer_beep(60);
        }
        prev_buttons = buttons;

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
