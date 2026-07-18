#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "leashlink_types.h"

typedef enum {
    LL_MSG_PAIR_REQ = 0x01,
    LL_MSG_PAIR_ACK = 0x02,
    LL_MSG_HEARTBEAT = 0x10,
    LL_MSG_COLLAR_TELEMETRY = 0x11,
    LL_MSG_HANDLE_TELEMETRY = 0x12,
    LL_MSG_ALERT_EVENT = 0x20,
    LL_MSG_CONTROL_CMD = 0x30,
    LL_MSG_CONTROL_ACK = 0x31,
    LL_MSG_CONFIG_SET = 0x40,
    LL_MSG_CONFIG_ACK = 0x41,
    LL_MSG_LINK_DIAG = 0x50,
} ll_msg_type_t;

esp_err_t espnow_handle_init(void);
ll_collar_telemetry_t espnow_handle_get_collar(void);
bool espnow_handle_link_ok(void);
esp_err_t espnow_handle_send_feedback(uint8_t cmd_type, uint16_t duration_ms);
