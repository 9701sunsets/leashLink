#include "cloud_service.h"

#include <stdio.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "distance_service.h"
#include "espnow_handle.h"
#include "gps_driver.h"
#include "heart_sensor.h"
#include "light_sensor.h"
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
    char payload[1024];
    ll_gps_fix_t gps = gps_get_latest();
    ll_light_sample_t light = {0};
    bool light_ok = light_sensor_read(&light) == ESP_OK;
    ll_heart_sensor_status_t heart_status = heart_sensor_get_status();
    ll_heart_raw_sample_t heart_raw = {0};
    bool heart_ok = heart_sensor_read_raw(&heart_raw) == ESP_OK && heart_raw.valid;
    bool link_ok = espnow_handle_link_ok();
    float distance_est_m = collar ? distance_service_estimate_from_rssi(collar->rssi_dbm) : 999.0f;
    const char *alert = link_ok ? "none" : "link_lost";

    snprintf(payload, sizeof(payload),
             "{\"protocol_version\":1,\"pair_id\":\"%s\",\"session_id\":\"%s\",\"ts_ms\":%lld,"
             "\"handle\":{\"tension_n\":%.2f,\"tension_peak_n\":%.2f,\"tension_stable\":%s,"
             "\"leash_locked\":%s,\"ambient_light_lux\":%d,\"ambient_light_raw\":%d,\"dark\":%s,"
             "\"battery_pct\":%d,"
             "\"heart\":{\"present\":%s,\"ok\":%s,\"ir\":%lu,\"red\":%lu,\"i2c_addr\":%u,\"part_id\":%u,\"int_level\":%d},"
             "\"gps\":{\"lat\":%.6f,\"lng\":%.6f,\"fix\":%s,\"accuracy_m\":%.1f}},"
             "\"collar\":{\"motion_state\":%d,\"steps\":%lu,\"accel_peak_g\":%.3f,"
             "\"confidence_pct\":%u,\"battery_pct\":%u,\"rssi_dbm\":%d,\"temp_c_x10\":%d,\"distance_est_m\":%.2f},"
             "\"location\":{\"lat\":%.6f,\"lng\":%.6f,\"fix\":%s,\"accuracy_m\":%.1f},"
             "\"link\":{\"espnow_ok\":%s},\"alert\":\"%s\"}",
             LL_PAIR_ID,
             session_service_get_id(),
             (long long)(esp_timer_get_time() / 1000),
             tension ? tension->tension_n : 0.0f,
             tension ? tension->tension_peak_n : 0.0f,
             tension && tension->stable ? "true" : "false",
             leash_state == LL_LEASH_LOCKED ? "true" : "false",
             light_ok ? light.lux_est : 0,
             light_ok ? light.raw : 0,
             light_ok && light.digital_dark ? "true" : "false",
             battery_pct,
             heart_status.present ? "true" : "false",
             heart_ok ? "true" : "false",
             (unsigned long)heart_raw.ir,
             (unsigned long)heart_raw.red,
             heart_status.i2c_addr,
             heart_status.part_id,
             heart_status.int_level,
             gps.lat,
             gps.lng,
             gps.fix ? "true" : "false",
             gps.accuracy_m,
             collar ? collar->motion_state : LL_MOTION_UNKNOWN,
             collar ? (unsigned long)collar->steps : 0,
             collar ? collar->accel_peak_g : 0.0f,
             collar ? collar->confidence_pct : 0,
             collar ? collar->battery_pct : 0,
             collar ? collar->rssi_dbm : 0,
             collar ? collar->temp_c_x10 : 0,
             distance_est_m,
             gps.lat,
             gps.lng,
             gps.fix ? "true" : "false",
             gps.accuracy_m,
             link_ok ? "true" : "false",
             alert);

    ESP_LOGI(TAG, "telemetry %s", payload);
    esp_err_t err = mqtt_client_ll_publish("leashlink/" LL_PAIR_ID "/telemetry", payload, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "telemetry publish failed: %s", esp_err_to_name(err));
    }
    return err;
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
    esp_err_t err = mqtt_client_ll_publish("leashlink/" LL_PAIR_ID "/event", payload, 1);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "event publish failed: %s", esp_err_to_name(err));
    }
    return err;
}
