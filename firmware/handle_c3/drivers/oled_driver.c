#include "oled_driver.h"

#include "esp_log.h"

static const char *TAG = "oled";

esp_err_t oled_init(void)
{
    ESP_LOGI(TAG, "OLED stub initialized. Integrate SSD1306/U8g2 here.");
    return ESP_OK;
}

esp_err_t oled_show_status(const ll_tension_sample_t *tension,
                           const ll_collar_telemetry_t *collar,
                           ll_safety_state_t safety_state,
                           int battery_pct)
{
    ESP_LOGI(TAG, "bat=%d%% tension=%.1fN peak=%.1fN motion=%d safety=%d",
             battery_pct,
             tension ? tension->tension_n : 0.0f,
             tension ? tension->tension_peak_n : 0.0f,
             collar ? collar->motion_state : LL_MOTION_UNKNOWN,
             safety_state);
    return ESP_OK;
}

esp_err_t oled_show_message(const char *line1, const char *line2)
{
    ESP_LOGI(TAG, "%s | %s", line1 ? line1 : "", line2 ? line2 : "");
    return ESP_OK;
}

