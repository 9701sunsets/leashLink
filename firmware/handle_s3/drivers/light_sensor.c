#include "light_sensor.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "light_sensor";
static adc_oneshot_unit_handle_t s_adc;

esp_err_t light_sensor_init(void)
{
    if (!s_adc) {
        adc_oneshot_unit_init_cfg_t init_cfg = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &s_adc));

        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, HANDLE_LIGHT_ADC_CHANNEL, &chan_cfg));
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
    if (!out || !s_adc) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc, HANDLE_LIGHT_ADC_CHANNEL, &raw));

    out->raw = raw;
    out->lux_est = raw / 4;
    out->digital_dark = gpio_get_level(HANDLE_LIGHT_DO_GPIO) == 0;
    return ESP_OK;
}
