#include "tension_service.h"

#include <math.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "hx711_driver.h"

static const char *TAG = "tension_service";
static ll_tension_sample_t s_latest;
static float s_filtered_n;

esp_err_t tension_service_init(void)
{
    ESP_ERROR_CHECK(hx711_init());
    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

esp_err_t tension_service_sample(ll_tension_sample_t *out)
{
    float n = 0.0f;
    esp_err_t err = hx711_read_newtons(&n);
    if (err != ESP_OK) {
        return err;
    }

    const float alpha = 0.25f;
    s_filtered_n = alpha * n + (1.0f - alpha) * s_filtered_n;
    if (s_filtered_n > s_latest.tension_peak_n) {
        s_latest.tension_peak_n = s_filtered_n;
    }

    s_latest.tension_n = s_filtered_n;
    s_latest.stable = fabsf(n - s_filtered_n) < 1.5f;
    s_latest.ts_ms = esp_timer_get_time() / 1000;
    if (out) {
        *out = s_latest;
    }
    return ESP_OK;
}

ll_tension_sample_t tension_service_get_latest(void)
{
    return s_latest;
}

