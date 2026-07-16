#include "buzzer_driver.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

/**
 * 初始化蜂鸣器
 */
esp_err_t buzzer_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << HANDLE_BUZZER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    return buzzer_stop();
}

/**
 * 使蜂鸣器发声
 * @param duration_ms 发声持续时间（毫秒）
 */
esp_err_t buzzer_beep(uint16_t duration_ms)
{
    gpio_set_level(HANDLE_BUZZER_GPIO, 0);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(HANDLE_BUZZER_GPIO, 1);
    }
    return ESP_OK;
}

/**
 * 停止蜂鸣器
 */
esp_err_t buzzer_stop(void)
{
    gpio_set_level(HANDLE_BUZZER_GPIO, 1);
    return ESP_OK;
}
