#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble_adv.h"
#include "buzzer_driver.h"
#include "espnow_handle.h"
#include "feedback_service.h"
#include "imu_driver.h"
#include "led_driver.h"
#include "motion_service.h"
#include "power_service.h"
#include "vibration_driver.h"

static const char *TAG = "collar_main";

void task_imu(void *arg);
void task_feedback(void *arg);

void app_main(void)
{
    ESP_LOGI(TAG, "leashLink collar C5 boot");

    ESP_ERROR_CHECK(power_service_init());
    ESP_ERROR_CHECK(imu_init());
    ESP_ERROR_CHECK(led_init());
    ESP_ERROR_CHECK(buzzer_init());
    ESP_ERROR_CHECK(vibration_init());
    ESP_ERROR_CHECK(motion_service_init());
    ESP_ERROR_CHECK(feedback_service_init());
    ESP_ERROR_CHECK(espnow_handle_init());
    ESP_ERROR_CHECK(ble_adv_init());

    xTaskCreatePinnedToCore(task_imu, "task_imu", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_feedback, "task_feedback", 4096, NULL, 4, NULL, 0);

    ESP_LOGI(TAG, "collar tasks started");
}
