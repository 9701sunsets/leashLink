#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LL_PROTOCOL_VERSION 1
#define LL_PAIR_ID "LL-P-0001"
#define LL_HANDLE_ID "LL-H-S3-0001"
#define LL_COLLAR_ID "LL-C-C5-0001"

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

typedef enum {
    LL_MOTION_IDLE = 0,// 静止
    LL_MOTION_WALK = 1,// 走路
    LL_MOTION_RUN = 2,// 跑步
    LL_MOTION_BURST = 3,// 突发拉扯
    LL_MOTION_SHAKE = 4,// 摇晃
    LL_MOTION_UNKNOWN = 255,// 未知
} ll_motion_state_t;

typedef enum {
    LL_FEEDBACK_STOP = 1,// 停止反馈
    LL_FEEDBACK_WARNING = 2,// 警告反馈
    LL_FEEDBACK_DANGER = 3,// 危险反馈
    LL_FEEDBACK_FIND_DOG = 4,// 寻找狗狗反馈
    LL_FEEDBACK_SET_LED_PATTERN = 5,// 设置LED模式反馈
    LL_FEEDBACK_ENTER_LOW_POWER = 6,// 进入低功耗反馈
} ll_feedback_cmd_type_t;

typedef struct {
    uint8_t motion_state;// 运动状态
    uint32_t steps;// 步数
    uint16_t accel_peak_mg;// 加速度峰值 (mg)
    uint8_t confidence_pct;// 置信度 (%)
    int8_t rssi_dbm;// 信号强度 (dBm)
    uint8_t battery_pct;// 电池电量 (%)
    int16_t temp_c_x10;// 温度 (°C * 10)
} ll_collar_telemetry_t;

typedef struct {
    uint16_t cmd_id;// 命令ID
    uint8_t cmd_type;// 命令类型
    uint16_t duration_ms;// 持续时间 (ms)
    int16_t param_a;// 参数A
    int16_t param_b;// 参数B
} ll_control_cmd_t;

#ifdef __cplusplus
}
#endif
