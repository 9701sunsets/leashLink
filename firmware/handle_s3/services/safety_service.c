#include "safety_service.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "safety";

/**
 * 默认安全配置
 */
static ll_safety_config_t s_cfg = {
    .warn_tension_n = 12.0f,
    .lock_tension_n = 20.0f,
    .accel_peak_g = 1.8f,
    .pre_alert_hold_ms = 180,
    .lock_hold_ms = 1000,
    .distance_warn_m = 10.0f,
    .distance_danger_m = 20.0f,
    .cooldown_ms = 2000,
};

static ll_safety_state_t s_state = LL_SAFETY_SAFE;
static int64_t s_state_since_ms;
static int64_t s_last_event_ms;

/**
 * 状态转换
 */
static void transition(ll_safety_state_t next, int64_t now_ms)
{
    if (s_state == next) {
        return;
    }
    ESP_LOGI(TAG, "state %d -> %d", s_state, next);
    s_state = next;
    s_state_since_ms = now_ms;
}

/**
 * 初始化安全服务
 * @param config 配置参数（可选）
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t safety_service_init(const ll_safety_config_t *config)
{
    if (config) {
        s_cfg = *config;
    }
    s_state = LL_SAFETY_SAFE;
    s_state_since_ms = esp_timer_get_time() / 1000;
    return ESP_OK;
}

/**
 * 获取当前安全状态
 */
ll_safety_state_t safety_service_get_state(void)
{
    return s_state;
}

/**
 * 评估安全状态
 * @param input 输入数据
 * @param event_out 输出事件（可选）
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t safety_service_eval(const ll_safety_input_t *input, ll_safety_event_t *event_out)
{
    if (!input) {
        return ESP_ERR_INVALID_ARG;
    }

    if (event_out) {
        memset(event_out, 0, sizeof(*event_out));
        event_out->type = LL_EVENT_NONE;
    }

    int64_t now = input->ts_ms > 0 ? input->ts_ms : esp_timer_get_time() / 1000;
    bool tension_watch = input->tension_n >= s_cfg.warn_tension_n;
    bool burst = input->tension_peak_n >= s_cfg.lock_tension_n &&
                 input->collar_accel_peak_g >= s_cfg.accel_peak_g;
    bool distance_danger = input->distance_est_m >= s_cfg.distance_danger_m ||
                           input->gps_fence_breach;
    bool distance_warn = input->distance_est_m >= s_cfg.distance_warn_m;

    switch (s_state) {
    case LL_SAFETY_SAFE:
        if (distance_danger) {
            transition(LL_SAFETY_DISTANCE_ALERT, now);
        } else if (tension_watch || input->motion_state == LL_MOTION_RUN) {
            transition(LL_SAFETY_WATCH, now);
        }
        break;

    case LL_SAFETY_WATCH:
        if (burst) {
            transition(LL_SAFETY_BURST_ALERT, now);
        } else if (distance_danger) {
            transition(LL_SAFETY_DISTANCE_ALERT, now);
        } else if (input->tension_n >= s_cfg.warn_tension_n &&
                   now - s_state_since_ms >= (int64_t)s_cfg.pre_alert_hold_ms) {
            transition(LL_SAFETY_PRE_ALERT, now);
        } else if (!tension_watch && !distance_warn) {
            transition(LL_SAFETY_SAFE, now);
        }
        break;

    case LL_SAFETY_PRE_ALERT:
        if (burst) {
            transition(LL_SAFETY_BURST_ALERT, now);
        } else if (!tension_watch) {
            transition(LL_SAFETY_SAFE, now);
        }
        break;

    case LL_SAFETY_BURST_ALERT:
        if (event_out && now - s_last_event_ms > 500) {
            event_out->type = LL_EVENT_BURST_PULL;
            event_out->severity = 3;
            event_out->tension_peak_n = input->tension_peak_n;
            event_out->accel_peak_g = input->collar_accel_peak_g;
            event_out->distance_m = input->distance_est_m;
            event_out->latency_ms = 0;
            event_out->ts_ms = now;
            s_last_event_ms = now;
        }
        transition(LL_SAFETY_LOCKED, now);
        break;

    case LL_SAFETY_LOCKED:
        if (now - s_state_since_ms >= (int64_t)s_cfg.lock_hold_ms &&
            input->tension_n < s_cfg.warn_tension_n) {
            transition(LL_SAFETY_RECOVERY, now);
        }
        break;

    case LL_SAFETY_RECOVERY:
        if (now - s_state_since_ms >= (int64_t)s_cfg.cooldown_ms) {
            transition(LL_SAFETY_SAFE, now);
        }
        break;

    case LL_SAFETY_DISTANCE_ALERT:
        if (event_out && now - s_last_event_ms > 3000) {
            event_out->type = input->gps_fence_breach ? LL_EVENT_FENCE_BREACH : LL_EVENT_DISTANCE_WARNING;
            event_out->severity = 2;
            event_out->distance_m = input->distance_est_m;
            event_out->ts_ms = now;
            s_last_event_ms = now;
        }
        if (!distance_danger && input->distance_est_m < s_cfg.distance_warn_m) {
            transition(LL_SAFETY_SAFE, now);
        }
        break;

    default:
        transition(LL_SAFETY_SAFE, now);
        break;
    }

    return ESP_OK;
}

