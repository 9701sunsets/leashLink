#include "vibration_driver.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

static const char *TAG = "vibration";

esp_err_t vibration_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << COLLAR_MOTOR_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    vibration_stop();
    ESP_LOGI(TAG, "vibration initialized");
    return ESP_OK;
}

esp_err_t vibration_run(uint16_t duration_ms)
{
    gpio_set_level(COLLAR_MOTOR_GPIO, 1);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(COLLAR_MOTOR_GPIO, 0);
    }
    return ESP_OK;
}

esp_err_t vibration_stop(void)
{
    gpio_set_level(COLLAR_MOTOR_GPIO, 0);
    return ESP_OK;
}
