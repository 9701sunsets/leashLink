#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ble_pairing.h"
#include "buzzer_driver.h"
#include "cloud_service.h"
#include "distance_service.h"
#include "espnow_handle.h"
#include "gps_driver.h"
#include "leash_control.h"
#include "motor_driver.h"
#include "oled_driver.h"
#include "power.h"
#include "safety_service.h"
#include "session_service.h"
#include "tension_service.h"

void task_tension(void *arg);
void task_safety(void *arg);
void task_actuator(void *arg);
void task_ui(void *arg);
void task_cloud(void *arg);

static const char *TAG = "app_main";

static void task_gps(void *arg)
{
    (void)arg;
    while (true) {
        gps_poll(NULL);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "leashLink handle_c3 boot");

    ESP_ERROR_CHECK(power_init());
    ESP_ERROR_CHECK(oled_init());
    ESP_ERROR_CHECK(buzzer_init());
    ESP_ERROR_CHECK(motor_init());
    ESP_ERROR_CHECK(leash_control_init());
    ESP_ERROR_CHECK(tension_service_init());
    ESP_ERROR_CHECK(gps_init());
    ESP_ERROR_CHECK(distance_service_init());
    ESP_ERROR_CHECK(safety_service_init(NULL));
    ESP_ERROR_CHECK(session_service_init());
    ESP_ERROR_CHECK(espnow_handle_init());
    ESP_ERROR_CHECK(ble_pairing_init());
    ESP_ERROR_CHECK(cloud_service_init());

    xTaskCreate(task_tension, "task_tension", 4096, NULL, 8, NULL);
    xTaskCreate(task_safety, "task_safety", 4096, NULL, 9, NULL);
    xTaskCreate(task_actuator, "task_actuator", 3072, NULL, 7, NULL);
    xTaskCreate(task_ui, "task_ui", 4096, NULL, 4, NULL);
    xTaskCreate(task_cloud, "task_cloud", 4096, NULL, 3, NULL);
    xTaskCreate(task_gps, "task_gps", 4096, NULL, 4, NULL);

    oled_show_message("leashLink", "handle ready");
}

