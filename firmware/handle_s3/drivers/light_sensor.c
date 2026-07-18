#include "light_sensor.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "power.h"

static const char *TAG = "light_sensor";
static bool s_initialized;

esp_err_t light_sensor_init(void)
{
    if (!s_initialized) {
        ESP_ERROR_CHECK(power_init());
        s_initialized = true;
    }

    gpio_config_t do_cfg = {
        .pin_bit_mask = 1ULL << HANDLE_LIGHT_DO_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&do_cfg));

    ESP_LOGI(TAG, "ready: AO=GPIO%d ADC1_CH%d DO=GPIO%d",
             HANDLE_LIGHT_AO_GPIO, HANDLE_LIGHT_ADC_CHANNEL, HANDLE_LIGHT_DO_GPIO);
    return ESP_OK;
}

esp_err_t light_sensor_read(ll_light_sample_t *out)
{
    if (!out || !s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw = 0;
    ESP_ERROR_CHECK(power_read_light_raw(&raw));

    out->raw = raw;
    out->lux_est = raw / 4;
    out->digital_dark = gpio_get_level(HANDLE_LIGHT_DO_GPIO) == 0;
    return ESP_OK;
}
