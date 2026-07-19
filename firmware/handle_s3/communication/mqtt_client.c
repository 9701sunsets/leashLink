#include "ll_mqtt_client.h"
#include "wifi_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"
#include "sdkconfig.h"

#include "espnow_handle.h"
#include "leash_control.h"
#include "leashlink_types.h"

static const char *TAG = "mqtt_client";
static esp_mqtt_client_handle_t s_client;
static bool s_connected;

static void publish_cmd_ack(const char *cmd_id, bool accepted, const char *node, const char *code, const char *message)
{
    if (!s_client || !s_connected) {
        return;
    }

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"cmd_id\":\"%s\",\"accepted\":%s,\"node\":\"%s\",\"code\":\"%s\",\"message\":\"%s\",\"ts_ms\":%lld}",
             cmd_id && cmd_id[0] ? cmd_id : "unknown",
             accepted ? "true" : "false",
             node,
             code,
             message,
             (long long)(esp_timer_get_time() / 1000));
    esp_mqtt_client_publish(s_client, "leashlink/" LL_PAIR_ID "/cmd_ack", payload, 0, 1, 0);
}

static void copy_json_string_value(const char *json, const char *key, char *out, size_t out_len)
{
    if (!json || !key || !out || out_len == 0) {
        return;
    }
    out[0] = '\0';

    const char *found = strstr(json, key);
    if (!found) {
        return;
    }
    const char *colon = strchr(found, ':');
    if (!colon) {
        return;
    }
    const char *start = strchr(colon, '"');
    if (!start) {
        return;
    }
    start++;
    const char *end = strchr(start, '"');
    if (!end || end <= start) {
        return;
    }

    size_t len = (size_t)(end - start);
    if (len >= out_len) {
        len = out_len - 1;
    }
    memcpy(out, start, len);
    out[len] = '\0';
}

static int copy_json_int_value(const char *json, const char *key, int fallback)
{
    const char *found = strstr(json, key);
    if (!found) {
        return fallback;
    }
    const char *colon = strchr(found, ':');
    if (!colon) {
        return fallback;
    }
    return atoi(colon + 1);
}

static void handle_cmd_payload(const char *payload)
{
    char cmd_id[48];
    char type[32];
    copy_json_string_value(payload, "\"cmd_id\"", cmd_id, sizeof(cmd_id));
    copy_json_string_value(payload, "\"type\"", type, sizeof(type));

    if (strcmp(type, "find_dog") == 0) {
        int duration_ms = copy_json_int_value(payload, "\"duration_ms\"", 10000);
        esp_err_t err = espnow_handle_send_feedback(LL_FEEDBACK_FIND_DOG, (uint16_t)duration_ms);
        publish_cmd_ack(cmd_id, err == ESP_OK, "collar", err == ESP_OK ? "ok" : "link_down", esp_err_to_name(err));
        return;
    }

    if (strcmp(type, "lock_leash") == 0) {
        esp_err_t err = leash_control_lock(0);
        publish_cmd_ack(cmd_id, err == ESP_OK, "handle", err == ESP_OK ? "ok" : "hardware_fault", esp_err_to_name(err));
        return;
    }

    if (strcmp(type, "unlock_leash") == 0) {
        esp_err_t err = leash_control_unlock();
        publish_cmd_ack(cmd_id, err == ESP_OK, "handle", err == ESP_OK ? "ok" : "hardware_fault", esp_err_to_name(err));
        return;
    }

    publish_cmd_ack(cmd_id, false, "handle", "invalid_payload", "unsupported command");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "MQTT connected");
        esp_mqtt_client_subscribe(s_client, "leashlink/" LL_PAIR_ID "/cmd", 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_ERROR:
        s_connected = false;
        if (event && event->error_handle) {
            ESP_LOGW(TAG,
                     "MQTT error type=%d esp_tls=%d sock_errno=%d tls_stack=%d connect_return=%d",
                     event->error_handle->error_type,
                     event->error_handle->esp_tls_last_esp_err,
                     event->error_handle->esp_transport_sock_errno,
                     event->error_handle->esp_tls_stack_err,
                     event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "MQTT error");
        }
        break;
    case MQTT_EVENT_DATA: {
        char topic[128];
        char payload[512];
        int topic_len = event->topic_len < (int)sizeof(topic) - 1 ? event->topic_len : (int)sizeof(topic) - 1;
        int data_len = event->data_len < (int)sizeof(payload) - 1 ? event->data_len : (int)sizeof(payload) - 1;
        memcpy(topic, event->topic, topic_len);
        topic[topic_len] = '\0';
        memcpy(payload, event->data, data_len);
        payload[data_len] = '\0';
        ESP_LOGI(TAG, "MQTT data topic=%s payload=%s", topic, payload);
        if (strcmp(topic, "leashlink/" LL_PAIR_ID "/cmd") == 0) {
            handle_cmd_payload(payload);
        }
        break;
    }
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

    ESP_LOGI(TAG, "MQTT init uri=%s", CONFIG_LEASHLINK_MQTT_URI);
    s_client = esp_mqtt_client_init(&config);
    if (!s_client) {
        ESP_LOGE(TAG, "MQTT client init failed");
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
    if (!s_client) {
        ESP_LOGW(TAG, "MQTT publish skipped: client not initialized topic=%s", topic);
        return ESP_ERR_INVALID_STATE;
    }
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "MQTT publish skipped: WiFi not connected topic=%s", topic);
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_connected) {
        ESP_LOGW(TAG, "MQTT publish skipped: MQTT not connected topic=%s", topic);
        return ESP_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, qos, 0);
    if (msg_id < 0) {
        ESP_LOGW(TAG, "MQTT publish failed topic=%s qos=%d", topic, qos);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "MQTT publish queued topic=%s qos=%d msg_id=%d", topic, qos, msg_id);
    return ESP_OK;
}
