#include "speaker_driver.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "sdkconfig.h"

static const char *TAG = "speaker";

esp_err_t speaker_init(void)
{
#if !CONFIG_COLLAR_ENABLE_SPEAKER_GPIO_TEST
    ESP_LOGW(TAG, "speaker GPIO test disabled; confirm PA/PDM pins before enabling");
    return ESP_OK;
#else
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << COLLAR_SPEAKER_PA_GPIO) |
                        (1ULL << COLLAR_SPEAKER_PDM_P_GPIO) |
                        (1ULL << COLLAR_SPEAKER_PDM_N_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    ESP_ERROR_CHECK(speaker_stop());
    ESP_LOGI(TAG, "ready: PA=GPIO%d PDM_P=GPIO%d PDM_N=GPIO%d",
             COLLAR_SPEAKER_PA_GPIO,
             COLLAR_SPEAKER_PDM_P_GPIO,
             COLLAR_SPEAKER_PDM_N_GPIO);
    return ESP_OK;
#endif
}

esp_err_t speaker_beep(uint16_t freq_hz, uint16_t duration_ms)
{
#if !CONFIG_COLLAR_ENABLE_SPEAKER_GPIO_TEST
    ESP_LOGI(TAG, "skip beep freq=%u duration=%u: GPIO test disabled", freq_hz, duration_ms);
    return ESP_OK;
#else
    if (freq_hz == 0 || duration_ms == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uint32_t half_period_us = 1000000UL / (uint32_t)freq_hz / 2UL;
    uint32_t cycles = ((uint32_t)duration_ms * 1000UL) / (half_period_us * 2UL);
    gpio_set_level(COLLAR_SPEAKER_PA_GPIO, 1);
    for (uint32_t i = 0; i < cycles; ++i) {
        gpio_set_level(COLLAR_SPEAKER_PDM_P_GPIO, 1);
        gpio_set_level(COLLAR_SPEAKER_PDM_N_GPIO, 0);
        esp_rom_delay_us(half_period_us);
        gpio_set_level(COLLAR_SPEAKER_PDM_P_GPIO, 0);
        gpio_set_level(COLLAR_SPEAKER_PDM_N_GPIO, 1);
        esp_rom_delay_us(half_period_us);
    }
    return speaker_stop();
#endif
}

esp_err_t speaker_stop(void)
{
#if !CONFIG_COLLAR_ENABLE_SPEAKER_GPIO_TEST
    return ESP_OK;
#else
    gpio_set_level(COLLAR_SPEAKER_PDM_P_GPIO, 0);
    gpio_set_level(COLLAR_SPEAKER_PDM_N_GPIO, 0);
    gpio_set_level(COLLAR_SPEAKER_PA_GPIO, 0);
    return ESP_OK;
#endif
}
