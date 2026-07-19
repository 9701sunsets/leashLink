#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ble_pairing.h"
#include "buzzer_driver.h"
#include "button_driver.h"
#include "cloud_service.h"
#include "distance_service.h"
#include "espnow_handle.h"
#include "gps_driver.h"
#include "heart_sensor.h"
#include "leash_control.h"
#include "light_sensor.h"
#include "motor_driver.h"
#include "oled_driver.h"
#include "power.h"
#include "safety_service.h"
#include "session_service.h"
#include "tension_service.h"
#include "wifi_manager.h"

void task_tension(void *arg);
void task_safety(void *arg);
void task_actuator(void *arg);
void task_ui(void *arg);
void task_cloud(void *arg);
void task_sensor_board(void *arg);
void task_handle_modules(void *arg);

static const char *TAG = "app_main";

#if !CONFIG_LEASHLINK_SENSOR_BRINGUP_ONLY
static void task_gps(void *arg)
{
    (void)arg;
    while (true) {
        gps_poll(NULL);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

static void init_wifi_from_saved_credentials(void)
{
    ESP_ERROR_CHECK(wifi_manager_init());

#if CONFIG_LEASHLINK_WIFI_SEED_ON_BOOT
    if (strlen(CONFIG_LEASHLINK_WIFI_SSID) > 0) {
        ESP_ERROR_CHECK(wifi_manager_set_credentials(
            CONFIG_LEASHLINK_WIFI_SSID,
            CONFIG_LEASHLINK_WIFI_PASSWORD));
        ESP_LOGI(TAG, "WiFi credentials seeded to NVS");
    }
#endif

    esp_err_t wifi_err = wifi_manager_connect_saved();
    if (wifi_err != ESP_OK) {
        ESP_LOGW(TAG, "WiFi hotspot not connected yet: %s", esp_err_to_name(wifi_err));
    }
}

static void init_sensor_bringup_board(void)
{
    ESP_ERROR_CHECK(session_service_init());
    ESP_ERROR_CHECK(button_driver_init());
    ESP_ERROR_CHECK(light_sensor_init());

    esp_err_t oled_err = oled_init();
    if (oled_err != ESP_OK) {
        ESP_LOGW(TAG, "OLED init failed: %s", esp_err_to_name(oled_err));
    }

    esp_err_t heart_err = heart_sensor_init();
    if (heart_err != ESP_OK) {
        ESP_LOGW(TAG, "heart sensor init failed: %s", esp_err_to_name(heart_err));
    }

    xTaskCreate(task_sensor_board, "task_sensor_board", 4096, NULL, 4, NULL);
    oled_show_message("leashLink", "sensors ready");
}

static void init_link_bringup(void)
{
    ESP_ERROR_CHECK(session_service_init());
    ESP_ERROR_CHECK(espnow_handle_init());
    ESP_ERROR_CHECK(ble_pairing_init());
    ESP_ERROR_CHECK(ble_pairing_start_scan());
    ESP_LOGI(TAG, "link bring-up started: ESP-NOW/BLE only");
}

#if !CONFIG_LEASHLINK_SENSOR_BRINGUP_ONLY
static void init_handle_module_bringup(void)
{
    ESP_ERROR_CHECK(session_service_init());
    ESP_ERROR_CHECK(power_init());
    ESP_ERROR_CHECK(button_driver_init());
    ESP_ERROR_CHECK(light_sensor_init());
    ESP_ERROR_CHECK(buzzer_init());
    ESP_ERROR_CHECK(gps_init());
    ESP_ERROR_CHECK(distance_service_init());
    ESP_ERROR_CHECK(leash_control_init());
    ESP_ERROR_CHECK(espnow_handle_init());
    ESP_ERROR_CHECK(ble_pairing_init());
    ESP_ERROR_CHECK(ble_pairing_start_scan());

    esp_err_t oled_err = oled_init();
    if (oled_err != ESP_OK) {
        ESP_LOGW(TAG, "OLED init failed: %s", esp_err_to_name(oled_err));
    }

    esp_err_t heart_err = heart_sensor_init();
    if (heart_err != ESP_OK) {
        ESP_LOGW(TAG, "heart sensor init failed: %s", esp_err_to_name(heart_err));
    }

    xTaskCreate(task_gps, "task_gps", 4096, NULL, 4, NULL);
    xTaskCreate(task_handle_modules, "task_handle_modules", 4096, NULL, 4, NULL);
    buzzer_beep(80);
    oled_show_lines("LL S3 READY", "MODULE TEST", "WAIT DATA", NULL);
    ESP_LOGI(TAG, "handle module bring-up started");
}

static void init_full_handle(void)
{
    ESP_ERROR_CHECK(power_init());
    ESP_ERROR_CHECK(oled_init());
    ESP_ERROR_CHECK(buzzer_init());
    ESP_ERROR_CHECK(button_driver_init());
    ESP_ERROR_CHECK(light_sensor_init());
    ESP_ERROR_CHECK(motor_init());
    ESP_ERROR_CHECK(leash_control_init());
    ESP_ERROR_CHECK(tension_service_init());
    ESP_ERROR_CHECK(gps_init());
    ESP_ERROR_CHECK(distance_service_init());
    ESP_ERROR_CHECK(safety_service_init(NULL));
    ESP_ERROR_CHECK(session_service_init());
    ESP_ERROR_CHECK(espnow_handle_init());
    ESP_ERROR_CHECK(ble_pairing_init());
    ESP_ERROR_CHECK(ble_pairing_start_scan());
    ESP_ERROR_CHECK(cloud_service_init());

    xTaskCreate(task_tension, "task_tension", 4096, NULL, 8, NULL);
    xTaskCreate(task_safety, "task_safety", 4096, NULL, 9, NULL);
    xTaskCreate(task_actuator, "task_actuator", 3072, NULL, 7, NULL);
    xTaskCreate(task_ui, "task_ui", 4096, NULL, 4, NULL);
    xTaskCreate(task_cloud, "task_cloud", 4096, NULL, 3, NULL);
    xTaskCreate(task_gps, "task_gps", 4096, NULL, 4, NULL);

    oled_show_message("leashLink", "handle ready");
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "leashLink handle_s3 boot");

    init_wifi_from_saved_credentials();

#if CONFIG_LEASHLINK_SENSOR_BRINGUP_ONLY
    init_sensor_bringup_board();
#elif CONFIG_LEASHLINK_LINK_BRINGUP_ONLY
    init_link_bringup();
#elif CONFIG_LEASHLINK_HANDLE_MODULE_BRINGUP_ONLY
    init_handle_module_bringup();
#else
    init_full_handle();
#endif
}
