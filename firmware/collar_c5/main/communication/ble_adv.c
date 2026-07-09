#include "ble_adv.h"

#include "esp_log.h"

static const char *TAG = "ble_adv";

esp_err_t ble_adv_init(void)
{
    ESP_LOGI(TAG, "BLE advertisement stub initialized");
    return ESP_OK;
}

esp_err_t ble_adv_start(void)
{
    ESP_LOGI(TAG, "BLE advertising started");
    return ESP_OK;
}
