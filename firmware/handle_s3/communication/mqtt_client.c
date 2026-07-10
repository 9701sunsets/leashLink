#include "mqtt_client.h"

#include "esp_log.h"

static const char *TAG = "mqtt_client";

esp_err_t mqtt_client_ll_init(void)
{
    ESP_LOGI(TAG, "MQTT stub initialized. Add Wi-Fi credentials and esp-mqtt URI here.");
    return ESP_OK;
}

esp_err_t mqtt_client_ll_publish(const char *topic, const char *payload, int qos)
{
    ESP_LOGI(TAG, "publish qos=%d topic=%s payload=%s", qos, topic, payload);
    return ESP_OK;
}

