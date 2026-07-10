#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "leashlink_types.h"

/**
 * ESP-NOW通信消息类型
 */
typedef enum {
    LL_MSG_PAIR_REQ = 0x01,// 配对请求
    LL_MSG_PAIR_ACK = 0x02,// 配对确认
    LL_MSG_HEARTBEAT = 0x10,// 心跳消息
    LL_MSG_COLLAR_TELEMETRY = 0x11,// 项圈遥测数据
    LL_MSG_HANDLE_TELEMETRY = 0x12,// 手柄遥测数据
    LL_MSG_ALERT_EVENT = 0x20,// 安全事件警报
    LL_MSG_CONTROL_CMD = 0x30,// 控制命令
} ll_msg_type_t;

esp_err_t espnow_handle_init(void);
ll_collar_telemetry_t espnow_handle_get_collar(void);
bool espnow_handle_link_ok(void);
esp_err_t espnow_handle_send_feedback(uint8_t cmd_type, uint16_t duration_ms);

