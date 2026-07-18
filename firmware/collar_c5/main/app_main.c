#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ble_adv.h"
#include "espnow_handle.h"
#include "feedback_service.h"
#include "imu_driver.h"
#include "led_driver.h"
#include "motion_service.h"
#include "power_service.h"
#include "speaker_driver.h"
#include "vibration_driver.h"
#include "wifi_manager.h"

static const char *TAG = "collar_main";

void task_imu(void *arg);
void task_feedback(void *arg);

static void init_wifi_from_saved_credentials(void)
{
    ESP_ERROR_CHECK(collar_wifi_init());

#if CONFIG_COLLAR_WIFI_SEED_ON_BOOT
    if (strlen(CONFIG_COLLAR_WIFI_SSID) > 0) {
        ESP_ERROR_CHECK(collar_wifi_set_credentials(CONFIG_COLLAR_WIFI_SSID,
                                                    CONFIG_COLLAR_WIFI_PASSWORD));
        ESP_LOGI(TAG, "WiFi credentials seeded to NVS");
    }
#endif

    esp_err_t wifi_err = collar_wifi_connect_saved();
    if (wifi_err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi not connected yet: %s", esp_err_to_name(wifi_err));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "leashLink collar C5 boot");

    init_wifi_from_saved_credentials();

    ESP_ERROR_CHECK(power_service_init());
    ESP_ERROR_CHECK(imu_init());
    ESP_ERROR_CHECK(led_init());
#if CONFIG_COLLAR_ENABLE_LEGACY_BUZZER
    ESP_ERROR_CHECK(buzzer_init());
#endif
    ESP_ERROR_CHECK(speaker_init());
    ESP_ERROR_CHECK(vibration_init());
    ESP_ERROR_CHECK(motion_service_init());
    ESP_ERROR_CHECK(feedback_service_init());
    ESP_ERROR_CHECK(espnow_handle_init());
    ESP_ERROR_CHECK(ble_adv_init());
    ESP_ERROR_CHECK(ble_adv_start());

    speaker_beep(1800, 120);

    xTaskCreatePinnedToCore(task_imu, "task_imu", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(task_feedback, "task_feedback", 4096, NULL, 4, NULL, 0);

    ESP_LOGI(TAG, "collar tasks started");
}
