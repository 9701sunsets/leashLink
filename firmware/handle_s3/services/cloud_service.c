#include "cloud_service.h"

#include <stdio.h>

#include "esp_log.h"

#include "ll_mqtt_client.h"
#include "session_service.h"

static const char *TAG = "cloud_service";

/**
 * 初始化云服务
 */
esp_err_t cloud_service_init(void)
{
    return mqtt_client_ll_init();
}

/**
 * 发布遥控器和项圈的遥测数据到云端
 * @param tension 张力样本
 * @param collar 项圈遥测数据
 * @param leash_state 牵引状态
 * @param battery_pct 电池百分比
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t cloud_service_publish_telemetry(const ll_tension_sample_t *tension,
                                          const ll_collar_telemetry_t *collar,
                                          ll_leash_state_t leash_state,
                                          int battery_pct)
{
    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"protocol_version\":1,\"pair_id\":\"%s\",\"session_id\":\"%s\","
             "\"handle\":{\"tension_n\":%.2f,\"leash_locked\":%s,\"battery_pct\":%d},"
             "\"collar\":{\"motion_state\":%d,\"steps\":%lu,\"battery_pct\":%u,\"rssi_dbm\":%d}}",
             LL_PAIR_ID,
             session_service_get_id(),
             tension ? tension->tension_n : 0.0f,
             leash_state == LL_LEASH_LOCKED ? "true" : "false",
             battery_pct,
             collar ? collar->motion_state : LL_MOTION_UNKNOWN,
             collar ? (unsigned long)collar->steps : 0,
             collar ? collar->battery_pct : 0,
             collar ? collar->rssi_dbm : 0);

    ESP_LOGI(TAG, "telemetry %s", payload);
    return mqtt_client_ll_publish("leashlink/" LL_PAIR_ID "/telemetry", payload, 0);
}

/**
 * 发布安全事件到云端
 * @param event 安全事件
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t cloud_service_publish_event(const ll_safety_event_t *event)
{
    if (!event || event->type == LL_EVENT_NONE) {
        return ESP_ERR_INVALID_ARG;
    }

    char payload[512];
    snprintf(payload, sizeof(payload),
             "{\"protocol_version\":1,\"pair_id\":\"%s\",\"session_id\":\"%s\","
             "\"type\":%d,\"severity\":%u,\"ts_ms\":%lld,"
             "\"metrics\":{\"tension_peak_n\":%.2f,\"accel_peak_g\":%.2f,\"distance_m\":%.2f}}",
             LL_PAIR_ID,
             session_service_get_id(),
             event->type,
             event->severity,
             (long long)event->ts_ms,
             event->tension_peak_n,
             event->accel_peak_g,
             event->distance_m);

    ESP_LOGW(TAG, "event %s", payload);
    session_service_count_event(event->type);
    return mqtt_client_ll_publish("leashlink/" LL_PAIR_ID "/event", payload, 1);
}
