#include "espnow_handle.h"

#include <string.h>

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "feedback_service.h"
#include "power_service.h"
#include "wifi_manager.h"

static const char *TAG = "espnow";

#define LL_FLAG_ACK_REQUIRED 0x01
#define LL_FLAG_IS_ACK       0x02

typedef struct __attribute__((packed)) {
    uint8_t magic[2];
    uint8_t version;
    uint8_t msg_type;
    uint8_t flags;
    uint16_t seq;
    uint32_t ts_ms;
    uint8_t payload_len;
} ll_frame_header_t;

typedef struct __attribute__((packed)) {
    uint8_t node_role;
    uint8_t battery_pct;
    uint8_t state;
    uint16_t error_code;
} ll_heartbeat_payload_t;

typedef struct __attribute__((packed)) {
    uint8_t motion_state;
    uint32_t steps;
    uint16_t accel_peak_mg;
    uint8_t confidence_pct;
    int8_t rssi_dbm;
    uint8_t battery_pct;
    int16_t temp_c_x10;
} ll_collar_payload_t;

typedef struct __attribute__((packed)) {
    uint16_t ack_seq;
    uint8_t status;
    uint8_t detail;
} ll_ack_payload_t;

static uint16_t s_seq;
static uint8_t s_broadcast_peer[ESP_NOW_ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

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

static esp_err_t send_frame(const uint8_t *peer_addr,
                            uint8_t msg_type,
                            uint8_t flags,
                            const void *payload,
                            uint8_t payload_len)
{
    uint8_t frame[sizeof(ll_frame_header_t) + 200 + 2] = {0};
    if (payload_len > 200 || (payload_len > 0 && !payload)) {
        return ESP_ERR_INVALID_ARG;
    }

    ll_frame_header_t *h = (ll_frame_header_t *)frame;
    h->magic[0] = 'L';
    h->magic[1] = 'L';
    h->version = LL_PROTOCOL_VERSION;
    h->msg_type = msg_type;
    h->flags = flags;
    h->seq = ++s_seq;
    h->ts_ms = (uint32_t)(esp_timer_get_time() / 1000);
    h->payload_len = payload_len;
    if (payload_len > 0) {
        memcpy(frame + sizeof(*h), payload, payload_len);
    }

    size_t frame_len = sizeof(*h) + payload_len + 2;
    uint16_t crc = crc16_ccitt(frame, frame_len - 2);
    frame[frame_len - 2] = crc & 0xFF;
    frame[frame_len - 1] = crc >> 8;

    esp_err_t err = esp_now_send(peer_addr ? peer_addr : s_broadcast_peer, frame, frame_len);
    ESP_LOGD(TAG, "send type=0x%02x seq=%u len=%u err=%s",
             msg_type, h->seq, payload_len, esp_err_to_name(err));
    return err;
}

static void send_ack(const uint8_t *peer_addr, uint16_t ack_seq, uint8_t status)
{
    ll_ack_payload_t ack = {
        .ack_seq = ack_seq,
        .status = status,
        .detail = 0,
    };
    send_frame(peer_addr, LL_MSG_CONTROL_ACK, LL_FLAG_IS_ACK, &ack, sizeof(ack));
}

static void handle_payload(const esp_now_recv_info_t *info,
                           const ll_frame_header_t *h,
                           const uint8_t *payload)
{
    if (h->msg_type == LL_MSG_CONTROL_CMD && h->payload_len >= sizeof(ll_control_cmd_t)) {
        ll_control_cmd_t cmd;
        memcpy(&cmd, payload, sizeof(cmd));
        ESP_LOGI(TAG, "recv CONTROL_CMD seq=%u cmd=%u duration=%u",
                 h->seq, cmd.cmd_type, cmd.duration_ms);
        esp_err_t err = feedback_service_handle_cmd(&cmd);
        if (h->flags & LL_FLAG_ACK_REQUIRED) {
            send_ack(info->src_addr, h->seq, err == ESP_OK ? 0 : 1);
        }
        return;
    }

    if (h->msg_type == LL_MSG_PAIR_REQ) {
        ESP_LOGI(TAG, "recv PAIR_REQ seq=%u from " MACSTR, h->seq, MAC2STR(info->src_addr));
        if (h->flags & LL_FLAG_ACK_REQUIRED) {
            send_frame(info->src_addr, LL_MSG_PAIR_ACK, LL_FLAG_IS_ACK, NULL, 0);
        }
        return;
    }

    if (h->msg_type == LL_MSG_HEARTBEAT) {
        ESP_LOGD(TAG, "recv HEARTBEAT seq=%u", h->seq);
        return;
    }

    ESP_LOGD(TAG, "recv unsupported type=0x%02x seq=%u", h->msg_type, h->seq);
}

static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (!info || !data || len < (int)(sizeof(ll_frame_header_t) + 2)) {
        return;
    }

    const ll_frame_header_t *h = (const ll_frame_header_t *)data;
    if (h->magic[0] != 'L' || h->magic[1] != 'L' || h->version != LL_PROTOCOL_VERSION) {
        ESP_LOGW(TAG, "drop invalid frame");
        return;
    }

    size_t expected = sizeof(ll_frame_header_t) + h->payload_len + 2;
    if ((size_t)len < expected) {
        ESP_LOGW(TAG, "drop short frame len=%d expected=%u", len, (unsigned)expected);
        return;
    }

    uint16_t rx_crc = (uint16_t)data[expected - 2] | ((uint16_t)data[expected - 1] << 8);
    if (crc16_ccitt(data, expected - 2) != rx_crc) {
        ESP_LOGW(TAG, "drop crc mismatch type=0x%02x seq=%u", h->msg_type, h->seq);
        return;
    }

    handle_payload(info, h, data + sizeof(*h));
}

esp_err_t espnow_handle_init(void)
{
    ESP_ERROR_CHECK(collar_wifi_init());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_err_t wifi_start_err = esp_wifi_start();
    if (wifi_start_err != ESP_OK && wifi_start_err != ESP_ERR_WIFI_STATE) {
        return wifi_start_err;
    }
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_recv));

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, s_broadcast_peer, ESP_NOW_ETH_ALEN);
    peer.channel = 0;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    ESP_LOGI(TAG, "initialized protocol=%u", LL_PROTOCOL_VERSION);
    return ESP_OK;
}

esp_err_t espnow_handle_send_telemetry(const ll_collar_telemetry_t *telemetry)
{
    if (!telemetry) {
        return ESP_ERR_INVALID_ARG;
    }

    ll_collar_payload_t payload = {
        .motion_state = telemetry->motion_state,
        .steps = telemetry->steps,
        .accel_peak_mg = telemetry->accel_peak_mg,
        .confidence_pct = telemetry->confidence_pct,
        .rssi_dbm = telemetry->rssi_dbm,
        .battery_pct = telemetry->battery_pct,
        .temp_c_x10 = telemetry->temp_c_x10,
    };

    return send_frame(s_broadcast_peer,
                      LL_MSG_COLLAR_TELEMETRY,
                      0,
                      &payload,
                      sizeof(payload));
}

esp_err_t espnow_handle_send_heartbeat(uint8_t state, uint16_t error_code)
{
    ll_heartbeat_payload_t heartbeat = {
        .node_role = 2,
        .battery_pct = power_service_get_battery_pct(),
        .state = state,
        .error_code = error_code,
    };
    return send_frame(s_broadcast_peer, LL_MSG_HEARTBEAT, 0, &heartbeat, sizeof(heartbeat));
}

esp_err_t espnow_handle_receive_control_cmd(const ll_control_cmd_t *cmd)
{
    if (!cmd) {
        return ESP_ERR_INVALID_ARG;
    }
    return feedback_service_handle_cmd(cmd);
}
