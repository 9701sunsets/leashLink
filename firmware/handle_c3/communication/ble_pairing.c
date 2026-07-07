#include "ble_pairing.h"

#include "esp_log.h"

static const char *TAG = "ble_pairing";

esp_err_t ble_pairing_init(void)
{
    ESP_LOGI(TAG, "BLE pairing stub initialized. Use NimBLE scan for collar advertisements.");
    return ESP_OK;
}

esp_err_t ble_pairing_start_scan(void)
{
    ESP_LOGI(TAG, "start scan stub");
    return ESP_OK;
}

