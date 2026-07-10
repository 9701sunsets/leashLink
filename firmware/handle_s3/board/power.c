#include "power.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#include "board_pins.h"

static const char *TAG = "power";
static adc_oneshot_unit_handle_t s_adc;

/**
 * 初始化电源管理
 */
esp_err_t power_init(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_cfg, &s_adc);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "adc init failed: %s", esp_err_to_name(err));
        return err;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, HANDLE_LIGHT_ADC_CHANNEL, &chan_cfg));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, HANDLE_BAT_ADC_CHANNEL, &chan_cfg));
    return ESP_OK;
}

/**
 * 获取电池电量百分比估计值（0-100）
 */
int power_get_battery_pct(void)
{
    int raw = 0;
    if (!s_adc || adc_oneshot_read(s_adc, HANDLE_BAT_ADC_CHANNEL, &raw) != ESP_OK) {
        return 100;
    }

    int pct = (raw - 1850) * 100 / (3000 - 1850);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

/**
 * 获取光照强度估计值（单位：lux）
 */
int power_get_light_lux_est(void)
{
    int raw = 0;
    if (!s_adc || adc_oneshot_read(s_adc, HANDLE_LIGHT_ADC_CHANNEL, &raw) != ESP_OK) {
        return 100;
    }

    return raw / 4;
}

