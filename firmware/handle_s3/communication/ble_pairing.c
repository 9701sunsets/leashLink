#include "ble_pairing.h"

#include "esp_log.h"
#include "leashlink_types.h"

static const char *TAG = "ble_pairing";

/**
 * 初始化BLE配对
 */
esp_err_t ble_pairing_init(void)
{
    ESP_LOGI(TAG, "BLE pairing scanner ready for handle_id=%s protocol=%u",
             LL_HANDLE_ID, LL_PROTOCOL_VERSION);
    return ESP_OK;
}

/**
 * 开始BLE扫描
 */
esp_err_t ble_pairing_start_scan(void)
{
    ESP_LOGI(TAG, "BLE scan stub started: filter manufacturer data magic=LL collar_id prefix=LL-C-C5");
    return ESP_OK;
}
