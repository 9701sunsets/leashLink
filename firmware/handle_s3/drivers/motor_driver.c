#include "motor_driver.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "board_pins.h"

#if CONFIG_LEASHLINK_HANDLE_MOTOR_ACTIVE_LOW
#define HANDLE_MOTOR_ON_LEVEL  0
#define HANDLE_MOTOR_OFF_LEVEL 1
#else
#define HANDLE_MOTOR_ON_LEVEL  1
#define HANDLE_MOTOR_OFF_LEVEL 0
#endif

esp_err_t motor_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << HANDLE_MOTOR_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    return motor_stop();
}

esp_err_t motor_vibrate(uint16_t duration_ms)
{
    gpio_set_level(HANDLE_MOTOR_GPIO, HANDLE_MOTOR_ON_LEVEL);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(HANDLE_MOTOR_GPIO, HANDLE_MOTOR_OFF_LEVEL);
    }
    return ESP_OK;
}

esp_err_t motor_stop(void)
{
    gpio_set_level(HANDLE_MOTOR_GPIO, HANDLE_MOTOR_OFF_LEVEL);
    return ESP_OK;
}
