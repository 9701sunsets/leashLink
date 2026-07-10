#include "motor_driver.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

esp_err_t motor_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << HANDLE_MOTOR_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    return motor_stop();
}

esp_err_t motor_vibrate(uint16_t duration_ms)
{
    gpio_set_level(HANDLE_MOTOR_GPIO, 1);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(HANDLE_MOTOR_GPIO, 0);
    }
    return ESP_OK;
}

esp_err_t motor_stop(void)
{
    gpio_set_level(HANDLE_MOTOR_GPIO, 0);
    return ESP_OK;
}

