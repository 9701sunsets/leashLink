#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LL_PROTOCOL_VERSION 1// 协议版本
#define LL_PAIR_ID "LL-P-0001"// 配对ID
#define LL_HANDLE_ID "LL-H-S3-0001"//board ID
#define LL_COLLAR_ID "LL-C-C5-0001"

typedef enum {
    LL_MOTION_IDLE = 0,// 静止
    LL_MOTION_WALK = 1,// 走路
    LL_MOTION_RUN = 2,// 跑步
    LL_MOTION_BURST = 3,// 突发拉扯
    LL_MOTION_SHAKE = 4,// 摇晃
    LL_MOTION_UNKNOWN = 255,// 未知
} ll_motion_state_t;

typedef enum {
    LL_LEASH_UNLOCKED = 0,// 牵引解锁
    LL_LEASH_LOCKED,// 牵引锁定
    LL_LEASH_RELEASING,// 牵引释放中
    LL_LEASH_RETRACTING,// 牵引收回中
    LL_LEASH_FAULT,// 牵引故障
} ll_leash_state_t;

typedef enum {
    LL_SAFETY_SAFE = 0,// 安全
    LL_SAFETY_WATCH,// 观察
    LL_SAFETY_PRE_ALERT,// 预警
    LL_SAFETY_BURST_ALERT,// 拉扯警告
    LL_SAFETY_LOCKED,// 锁定
    LL_SAFETY_RECOVERY,// 恢复
    LL_SAFETY_DISTANCE_ALERT,// 距离警告
    LL_SAFETY_FAULT_SAFE,// 故障安全
} ll_safety_state_t;

typedef enum {
    LL_EVENT_NONE = 0,// 无事件
    LL_EVENT_BURST_PULL = 1,// 拉扯事件
    LL_EVENT_DISTANCE_WARNING = 2,// 距离警告事件
    LL_EVENT_FENCE_BREACH = 3,// 围栏越界事件
    LL_EVENT_FATIGUE_DETECTED = 4,// 疲劳检测事件
    LL_EVENT_LOW_BATTERY = 5,// 低电量事件
    LL_EVENT_LINK_LOST = 6,// 链路丢失事件
} ll_event_type_t;

typedef struct {
    float tension_n;// 张力（牛顿）
    float tension_peak_n;// 峰值张力（牛顿）
    bool stable;// 张力是否稳定
    int64_t ts_ms;// 时间戳（毫秒）
} ll_tension_sample_t;

typedef struct {
    ll_motion_state_t motion_state;// 运动状态
    uint32_t steps;// 步数
    float accel_peak_g;// 加速度峰值（g）
    uint8_t confidence_pct;// 置信度（百分比）
    int8_t rssi_dbm;// 接收信号强度指示（dBm）
    uint8_t battery_pct;// 电池电量（百分比）
    int64_t ts_ms;// 时间戳（毫秒）
} ll_collar_telemetry_t;

typedef struct {
    bool fix;// GPS定位是否有效
    double lat;// 纬度
    double lng;// 经度
    float accuracy_m;// 定位精度（米）
    int64_t ts_ms;// 时间戳（毫秒）
} ll_gps_fix_t;

typedef struct {
    float warn_tension_n;// 警告张力（牛顿）
    float lock_tension_n;// 锁定张力（牛顿）
    float accel_peak_g;// 加速度峰值（g）
    uint32_t pre_alert_hold_ms;// 预警保持时间（毫秒）
    uint32_t lock_hold_ms;// 锁定保持时间（毫秒）
    float distance_warn_m;// 距离警告（米）
    float distance_danger_m;// 距离危险（米）
    uint32_t cooldown_ms;// 冷却时间（毫秒）
} ll_safety_config_t;

typedef struct {
    ll_event_type_t type;// 事件类型
    uint8_t severity;// 严重程度
    float tension_peak_n;// 峰值张力（牛顿）
    float accel_peak_g;// 加速度峰值（g）
    float distance_m;// 距离（米）
    uint32_t latency_ms;// 延迟时间（毫秒）
    int64_t ts_ms;// 时间戳（毫秒）
} ll_safety_event_t;

typedef struct {
    float tension_n;// 张力（牛顿）
    float tension_peak_n;// 峰值张力（牛顿）
    float collar_accel_peak_g;// 胸围加速度峰值（g）
    ll_motion_state_t motion_state;// 运动状态
    float distance_est_m;// 估计距离（米）
    bool gps_fence_breach;// GPS围栏越界
    bool link_ok;// 链路是否正常
    int64_t ts_ms;// 时间戳（毫秒）
} ll_safety_input_t;

#ifdef __cplusplus
}
#endif
