#include "ll_mqtt_client.h"
#include "wifi_manager.h"

#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

static const char *TAG = "mqtt_client";
static esp_mqtt_client_handle_t s_client;
static bool s_connected;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    (void)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "MQTT connected");
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_ERROR:
        s_connected = false;
        ESP_LOGW(TAG, "MQTT error");
        break;
    default:
        break;
    }
}


/**
 * 初始化MQTT客户端
 */
esp_err_t mqtt_client_ll_init(void)
{
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "skip MQTT init: WiFi not connected");
        return ESP_ERR_INVALID_STATE;
    }

    esp_mqtt_client_config_t config = {
        .broker.address.uri = CONFIG_LEASHLINK_MQTT_URI,
    };

    s_client = esp_mqtt_client_init(&config);
    if (!s_client) {
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));

    return esp_mqtt_client_start(s_client);
}

/**
 * 发布MQTT消息
 * @param topic 主题
 * @param payload 消息负载
 * @param qos QoS等级
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t mqtt_client_ll_publish(const char *topic, const char *payload, int qos)
{
    if (!s_client || !wifi_manager_is_connected() || !s_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, qos, 0);
    return msg_id >= 0 ? ESP_OK : ESP_FAIL;
}
