#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LL_MOTION_IDLE = 0,
    LL_MOTION_WALK = 1,
    LL_MOTION_RUN = 2,
    LL_MOTION_BURST = 3,
    LL_MOTION_SHAKE = 4,
    LL_MOTION_UNKNOWN = 255,
} ll_motion_state_t;

typedef enum {
    LL_FEEDBACK_STOP = 1,
    LL_FEEDBACK_WARNING = 2,
    LL_FEEDBACK_DANGER = 3,
    LL_FEEDBACK_FIND_DOG = 4,
    LL_FEEDBACK_SET_LED_PATTERN = 5,
    LL_FEEDBACK_ENTER_LOW_POWER = 6,
} ll_feedback_cmd_type_t;

typedef struct {
    uint8_t motion_state;
    uint32_t steps;
    uint16_t accel_peak_mg;
    uint8_t confidence_pct;
    int8_t rssi_dbm;
    uint8_t battery_pct;
    int16_t temp_c_x10;
} ll_collar_telemetry_t;

typedef struct {
    uint16_t cmd_id;
    uint8_t cmd_type;
    uint16_t duration_ms;
    int16_t param_a;
    int16_t param_b;
} ll_control_cmd_t;

#ifdef __cplusplus
}
#endif
