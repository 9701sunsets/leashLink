#include "ble_adv.h"

#include <stdint.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_random.h"
#include "leashlink_types.h"

static const char *TAG = "ble_adv";

typedef struct __attribute__((packed)) {
    uint8_t magic[2];
    uint8_t protocol_version;
    uint32_t random_code;
    char collar_id[16];
} ll_ble_adv_payload_t;

static ll_ble_adv_payload_t s_adv_payload;

/**
 * 初始化BLE广播
 */
esp_err_t ble_adv_init(void)
{
    s_adv_payload.magic[0] = 'L';
    s_adv_payload.magic[1] = 'L';
    s_adv_payload.protocol_version = LL_PROTOCOL_VERSION;
    s_adv_payload.random_code = esp_random();
    snprintf(s_adv_payload.collar_id, sizeof(s_adv_payload.collar_id), "%s", LL_COLLAR_ID);
    ESP_LOGI(TAG, "BLE adv payload ready collar_id=%s version=%u code=%lu",
             s_adv_payload.collar_id,
             s_adv_payload.protocol_version,
             (unsigned long)s_adv_payload.random_code);
    return ESP_OK;
}

/**
 * 启动BLE广播
 */
esp_err_t ble_adv_start(void)
{
    ESP_LOGI(TAG, "BLE advertising stub started: magic=%c%c collar_id=%s version=%u code=%lu",
             s_adv_payload.magic[0],
             s_adv_payload.magic[1],
             s_adv_payload.collar_id,
             s_adv_payload.protocol_version,
             (unsigned long)s_adv_payload.random_code);
    return ESP_OK;
}
