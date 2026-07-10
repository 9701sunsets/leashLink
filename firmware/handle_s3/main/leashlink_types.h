#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LL_PROTOCOL_VERSION 1
#define LL_PAIR_ID "LL-P-0001"
#define LL_HANDLE_ID "LL-H-C3-0001"

typedef enum {
    LL_MOTION_IDLE = 0,
    LL_MOTION_WALK = 1,
    LL_MOTION_RUN = 2,
    LL_MOTION_BURST = 3,
    LL_MOTION_SHAKE = 4,
    LL_MOTION_UNKNOWN = 255,
} ll_motion_state_t;

typedef enum {
    LL_LEASH_UNLOCKED = 0,
    LL_LEASH_LOCKED,
    LL_LEASH_RELEASING,
    LL_LEASH_RETRACTING,
    LL_LEASH_FAULT,
} ll_leash_state_t;

typedef enum {
    LL_SAFETY_SAFE = 0,
    LL_SAFETY_WATCH,
    LL_SAFETY_PRE_ALERT,
    LL_SAFETY_BURST_ALERT,
    LL_SAFETY_LOCKED,
    LL_SAFETY_RECOVERY,
    LL_SAFETY_DISTANCE_ALERT,
    LL_SAFETY_FAULT_SAFE,
} ll_safety_state_t;

typedef enum {
    LL_EVENT_NONE = 0,
    LL_EVENT_BURST_PULL = 1,
    LL_EVENT_DISTANCE_WARNING = 2,
    LL_EVENT_FENCE_BREACH = 3,
    LL_EVENT_FATIGUE_DETECTED = 4,
    LL_EVENT_LOW_BATTERY = 5,
    LL_EVENT_LINK_LOST = 6,
} ll_event_type_t;

typedef struct {
    float tension_n;
    float tension_peak_n;
    bool stable;
    int64_t ts_ms;
} ll_tension_sample_t;

typedef struct {
    ll_motion_state_t motion_state;
    uint32_t steps;
    float accel_peak_g;
    uint8_t confidence_pct;
    int8_t rssi_dbm;
    uint8_t battery_pct;
    int64_t ts_ms;
} ll_collar_telemetry_t;

typedef struct {
    bool fix;
    double lat;
    double lng;
    float accuracy_m;
    int64_t ts_ms;
} ll_gps_fix_t;

typedef struct {
    float warn_tension_n;
    float lock_tension_n;
    float accel_peak_g;
    uint32_t pre_alert_hold_ms;
    uint32_t lock_hold_ms;
    float distance_warn_m;
    float distance_danger_m;
    uint32_t cooldown_ms;
} ll_safety_config_t;

typedef struct {
    ll_event_type_t type;
    uint8_t severity;
    float tension_peak_n;
    float accel_peak_g;
    float distance_m;
    uint32_t latency_ms;
    int64_t ts_ms;
} ll_safety_event_t;

typedef struct {
    float tension_n;
    float tension_peak_n;
    float collar_accel_peak_g;
    ll_motion_state_t motion_state;
    float distance_est_m;
    bool gps_fence_breach;
    bool link_ok;
    int64_t ts_ms;
} ll_safety_input_t;

#ifdef __cplusplus
}
#endif

