#include "espnow_handle.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "espnow_handle";

/**
 * ESP-NOW协议版本
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[2];
    uint8_t version;
    uint8_t msg_type;
    uint8_t flags;
    uint16_t seq;
    uint32_t ts_ms;
    uint8_t payload_len;
} ll_frame_header_t;

/**
 * 项圈遥测数据负载结构
 */
typedef struct __attribute__((packed)) {
    uint8_t motion_state;// 运动状态
    uint32_t steps;// 步数
    uint16_t accel_peak_mg;// 加速度峰值 (mg)
    uint8_t confidence_pct;// 置信度 (%)
    int8_t rssi_dbm;// 信号强度 (dBm)
    uint8_t battery_pct;// 电池电量 (%)
    int16_t temp_c_x10;// 温度 (°C * 10)
} ll_collar_payload_t;

/**
 * 控制命令负载结构
 */
typedef struct __attribute__((packed)) {
    uint16_t cmd_id;// 命令ID
    uint8_t cmd_type;// 命令类型
    uint16_t duration_ms;// 持续时间 (ms)
    int16_t param_a;// 参数A
    int16_t param_b;// 参数B
} ll_control_payload_t;

typedef struct __attribute__((packed)) {
    uint16_t ack_seq;
    uint8_t status;
    uint8_t detail;
} ll_ack_payload_t;

/**
 * 最新的项圈遥测数据
 */
static ll_collar_telemetry_t s_collar = {
    .motion_state = LL_MOTION_UNKNOWN,
    .rssi_dbm = -100,
};
/**
 * 上次接收ESP-NOW消息的时间戳（毫秒）
 */
static int64_t s_last_rx_ms;
/**
 * 序列号，用于命令反馈
 */
static uint16_t s_seq;
/**
 * 广播地址（用于发送ESP-NOW消息）
 */
static uint8_t s_broadcast_peer[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/**
 * 计算CRC16-CCITT校验码
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16校验码
 */
static uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

/**
 * ESP-NOW接收回调
 */
static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (!info || !data || len < (int)(sizeof(ll_frame_header_t) + 2)) {
        return;
    }

    const ll_frame_header_t *h = (const ll_frame_header_t *)data;
    if (h->magic[0] != 'L' || h->magic[1] != 'L' || h->version != LL_PROTOCOL_VERSION) {
        return;
    }

    size_t expected = sizeof(ll_frame_header_t) + h->payload_len + 2;
    if ((size_t)len < expected) {
        return;
    }

    uint16_t rx_crc = (uint16_t)data[expected - 2] | ((uint16_t)data[expected - 1] << 8);
    if (crc16_ccitt(data, expected - 2) != rx_crc) {
        ESP_LOGW(TAG, "crc mismatch");
        return;
    }

    const uint8_t *payload = data + sizeof(ll_frame_header_t);
    if (h->msg_type == LL_MSG_COLLAR_TELEMETRY && h->payload_len >= sizeof(ll_collar_payload_t)) {
        ll_collar_payload_t p;
        memcpy(&p, payload, sizeof(p));
        s_collar.motion_state = (ll_motion_state_t)p.motion_state;
        s_collar.steps = p.steps;
        s_collar.accel_peak_g = (float)p.accel_peak_mg / 1000.0f;
        s_collar.confidence_pct = p.confidence_pct;
        s_collar.rssi_dbm = info->rx_ctrl ? info->rx_ctrl->rssi : p.rssi_dbm;
        s_collar.battery_pct = p.battery_pct;
        s_collar.ts_ms = esp_timer_get_time() / 1000;
        s_last_rx_ms = s_collar.ts_ms;
        ESP_LOGI(TAG, "collar telemetry seq=%u rssi=%d motion=%u steps=%lu battery=%u",
                 h->seq,
                 s_collar.rssi_dbm,
                 s_collar.motion_state,
                 (unsigned long)s_collar.steps,
                 s_collar.battery_pct);
    } else if (h->msg_type == LL_MSG_HEARTBEAT) {
        s_last_rx_ms = esp_timer_get_time() / 1000;
        ESP_LOGI(TAG, "heartbeat seq=%u from " MACSTR, h->seq, MAC2STR(info->src_addr));
    } else if (h->msg_type == LL_MSG_CONTROL_ACK && h->payload_len >= sizeof(ll_ack_payload_t)) {
        ll_ack_payload_t ack;
        memcpy(&ack, payload, sizeof(ack));
        ESP_LOGI(TAG, "control ack ack_seq=%u status=%u detail=%u",
                 ack.ack_seq, ack.status, ack.detail);
    } else if (h->msg_type == LL_MSG_PAIR_ACK) {
        ESP_LOGI(TAG, "pair ack seq=%u from " MACSTR, h->seq, MAC2STR(info->src_addr));
    }
}

/**
 * 初始化ESP-NOW通信
 */
esp_err_t espnow_handle_init(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, s_broadcast_peer, ESP_NOW_ETH_ALEN);
    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    esp_now_add_peer(&peer);

    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

/**
 * 获取最新的项圈遥测数据
 * @return 最新的项圈遥测数据
 */
ll_collar_telemetry_t espnow_handle_get_collar(void)
{
    return s_collar;
}

/**
 * 检查ESP-NOW链路是否正常
 * @return true if link is OK, false otherwise
 */
bool espnow_handle_link_ok(void)
{
    int64_t now = esp_timer_get_time() / 1000;
    return s_last_rx_ms > 0 && now - s_last_rx_ms < 5000;
}

/**
 * 发送反馈命令
 * @param cmd_type 命令类型
 * @param duration_ms 持续时间（毫秒）
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t espnow_handle_send_feedback(uint8_t cmd_type, uint16_t duration_ms)
{
    uint8_t frame[sizeof(ll_frame_header_t) + sizeof(ll_control_payload_t) + 2] = {0};
    ll_frame_header_t *h = (ll_frame_header_t *)frame;
    ll_control_payload_t payload = {
        .cmd_id = ++s_seq,
        .cmd_type = cmd_type,
        .duration_ms = duration_ms,
    };

    h->magic[0] = 'L';
    h->magic[1] = 'L';
    h->version = LL_PROTOCOL_VERSION;
    h->msg_type = LL_MSG_CONTROL_CMD;
    h->flags = 0x01;
    h->seq = s_seq;
    h->ts_ms = (uint32_t)(esp_timer_get_time() / 1000);
    h->payload_len = sizeof(payload);
    memcpy(frame + sizeof(*h), &payload, sizeof(payload));
    uint16_t crc = crc16_ccitt(frame, sizeof(frame) - 2);
    frame[sizeof(frame) - 2] = crc & 0xFF;
    frame[sizeof(frame) - 1] = crc >> 8;
    ESP_LOGI(TAG, "send control seq=%u cmd=%u duration=%u",
             h->seq, payload.cmd_type, payload.duration_ms);
    return esp_now_send(s_broadcast_peer, frame, sizeof(frame));
}
