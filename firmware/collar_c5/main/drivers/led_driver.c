#include "led_driver.h"

#include "esp_log.h"
#include "driver/gpio.h"

#include "board_pins.h"

static const char *TAG = "led";

esp_err_t led_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << COLLAR_LED_DATA_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    ESP_LOGI(TAG, "LED stub initialized");
    return ESP_OK;
}

esp_err_t led_set_pattern(uint8_t pattern)
{
    (void)pattern;
    ESP_LOGI(TAG, "set pattern=%u", pattern);
    return ESP_OK;
}

esp_err_t led_stop(void)
{
    ESP_LOGI(TAG, "led stop");
    return ESP_OK;
}
