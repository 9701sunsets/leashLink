#include "espnow_handle.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "feedback_service.h"

static const char *TAG = "espnow";

static void on_recv(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    (void)mac_addr;
    if (len < (int)sizeof(ll_control_cmd_t)) {
        return;
    }

    ll_control_cmd_t cmd;
    memcpy(&cmd, data, sizeof(cmd));
    ESP_LOGI(TAG, "recv control cmd type=%u", cmd.cmd_type);
    feedback_service_handle_cmd(&cmd);
}

esp_err_t espnow_handle_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));

    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

esp_err_t espnow_handle_send_telemetry(const ll_collar_telemetry_t *telemetry)
{
    if (!telemetry) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_err_t err = esp_now_send(broadcast, (const uint8_t *)telemetry, sizeof(*telemetry));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "send telemetry failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t espnow_handle_receive_control_cmd(const ll_control_cmd_t *cmd)
{
    if (!cmd) {
        return ESP_ERR_INVALID_ARG;
    }
    return feedback_service_handle_cmd(cmd);
}
