#include "motor_driver.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

/**
 * 初始化电机驱动
 */
esp_err_t motor_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << HANDLE_MOTOR_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    return motor_stop();
}

/**
 * 使电机振动
 * @param duration_ms 振动持续时间（毫秒）
 */
esp_err_t motor_vibrate(uint16_t duration_ms)
{
    gpio_set_level(HANDLE_MOTOR_GPIO, 1);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(HANDLE_MOTOR_GPIO, 0);
    }
    return ESP_OK;
}

/**
 * 停止电机
 */
esp_err_t motor_stop(void)
{
    gpio_set_level(HANDLE_MOTOR_GPIO, 0);
    return ESP_OK;
}

