#include "wifi_manager.h"

#include <string.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "nvs_flash.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_MAX_RETRY 10

static const char *TAG = "collar_wifi";
static EventGroupHandle_t s_wifi_events;
static esp_netif_t *s_sta_netif;
static int s_retry_count;
static bool s_connected;
static bool s_initialized;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT);
        if (s_retry_count < WIFI_MAX_RETRY) {
            ++s_retry_count;
            ESP_LOGW(TAG, "disconnected, retry %d/%d", s_retry_count, WIFI_MAX_RETRY);
            esp_wifi_connect();
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        s_retry_count = 0;
        s_connected = true;
        ESP_LOGI(TAG, "connected ip=" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

esp_err_t collar_wifi_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, TAG, "nvs init failed");

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init failed");
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    if (!s_wifi_events) {
        s_wifi_events = xEventGroupCreate();
    }
    if (!s_sta_netif) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
        return err;
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    s_initialized = true;
    return ESP_OK;
}

esp_err_t collar_wifi_set_credentials(const char *ssid, const char *password)
{
    if (!ssid || strlen(ssid) == 0 || strlen(ssid) > 32 || !password || strlen(password) > 64) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs;
    ESP_RETURN_ON_ERROR(nvs_open("wifi", NVS_READWRITE, &nvs), TAG, "open nvs failed");
    ESP_ERROR_CHECK(nvs_set_str(nvs, "ssid", ssid));
    ESP_ERROR_CHECK(nvs_set_str(nvs, "password", password));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);
    return ESP_OK;
}

esp_err_t collar_wifi_connect_saved(void)
{
    char ssid[33] = {0};
    char password[65] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t password_len = sizeof(password);

    nvs_handle_t nvs;
    ESP_RETURN_ON_ERROR(nvs_open("wifi", NVS_READONLY, &nvs), TAG, "open nvs failed");
    esp_err_t err = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    if (err == ESP_OK) {
        err = nvs_get_str(nvs, "password", password, &password_len);
    }
    nvs_close(nvs);
    ESP_RETURN_ON_ERROR(err, TAG, "load credentials failed");

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_LOGI(TAG, "connecting ssid=%s", ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_events, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(15000));
    return (bits & WIFI_CONNECTED_BIT) ? ESP_OK : ESP_ERR_TIMEOUT;
}

bool collar_wifi_is_connected(void)
{
    return s_connected;
}
