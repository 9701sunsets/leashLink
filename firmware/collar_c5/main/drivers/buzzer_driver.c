#include "buzzer_driver.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_pins.h"

static const char *TAG = "buzzer";

/**
 * 初始化蜂鸣器驱动
 */
esp_err_t buzzer_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << COLLAR_BUZZER_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    buzzer_stop();
    ESP_LOGI(TAG, "buzzer initialized");
    return ESP_OK;
}

/**
 * 蜂鸣器发声
 * @param duration_ms 持续时间（毫秒）
 */
esp_err_t buzzer_beep(uint16_t duration_ms)
{
    gpio_set_level(COLLAR_BUZZER_GPIO, 1);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(COLLAR_BUZZER_GPIO, 0);
    }
    return ESP_OK;
}

/**
 * 停止蜂鸣器
 */
esp_err_t buzzer_stop(void)
{
    gpio_set_level(COLLAR_BUZZER_GPIO, 0);
    return ESP_OK;
}
