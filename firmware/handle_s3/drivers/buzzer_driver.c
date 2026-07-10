#include "buzzer_driver.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

esp_err_t buzzer_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << HANDLE_BUZZER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    return buzzer_stop();
}

esp_err_t buzzer_beep(uint16_t duration_ms)
{
    gpio_set_level(HANDLE_BUZZER_GPIO, 1);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(HANDLE_BUZZER_GPIO, 0);
    }
    return ESP_OK;
}

esp_err_t buzzer_stop(void)
{
    gpio_set_level(HANDLE_BUZZER_GPIO, 0);
    return ESP_OK;
}

