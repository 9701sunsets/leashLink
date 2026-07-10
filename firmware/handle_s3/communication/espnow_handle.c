#include "espnow_handle.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "espnow_handle";

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
    uint8_t motion_state;
    uint32_t steps;
    uint16_t accel_peak_mg;
    uint8_t confidence_pct;
    int8_t rssi_dbm;
    uint8_t battery_pct;
    int16_t temp_c_x10;
} ll_collar_payload_t;

typedef struct __attribute__((packed)) {
    uint16_t cmd_id;
    uint8_t cmd_type;
    uint16_t duration_ms;
    int16_t param_a;
    int16_t param_b;
} ll_control_payload_t;

static ll_collar_telemetry_t s_collar = {
    .motion_state = LL_MOTION_UNKNOWN,
    .rssi_dbm = -100,
};
static int64_t s_last_rx_ms;
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

static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;
    if (!data || len < (int)(sizeof(ll_frame_header_t) + 2)) {
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
        s_collar.rssi_dbm = p.rssi_dbm;
        s_collar.battery_pct = p.battery_pct;
        s_collar.ts_ms = esp_timer_get_time() / 1000;
        s_last_rx_ms = s_collar.ts_ms;
    } else if (h->msg_type == LL_MSG_HEARTBEAT) {
        s_last_rx_ms = esp_timer_get_time() / 1000;
    }
}

esp_err_t espnow_handle_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

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

ll_collar_telemetry_t espnow_handle_get_collar(void)
{
    return s_collar;
}

bool espnow_handle_link_ok(void)
{
    int64_t now = esp_timer_get_time() / 1000;
    return s_last_rx_ms > 0 && now - s_last_rx_ms < 5000;
}

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
    return esp_now_send(s_broadcast_peer, frame, sizeof(frame));
}

