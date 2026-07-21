#include "vibration_driver.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "board_pins.h"

static const char *TAG = "vibration";

#if CONFIG_COLLAR_VIBRATION_ACTIVE_LOW
#define VIBRATION_ON_LEVEL  0
#define VIBRATION_OFF_LEVEL 1
#else
#define VIBRATION_ON_LEVEL  1
#define VIBRATION_OFF_LEVEL 0
#endif

/**
 * 初始化振动电机驱动
 */
esp_err_t vibration_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << COLLAR_MOTOR_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    vibration_stop();
    ESP_LOGI(TAG, "vibration initialized");
    return ESP_OK;
}

/**
 * 运行振动电机
 * @param duration_ms 持续时间（毫秒）
 */
esp_err_t vibration_run(uint16_t duration_ms)
{
    gpio_set_level(COLLAR_MOTOR_GPIO, VIBRATION_ON_LEVEL);
    if (duration_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(COLLAR_MOTOR_GPIO, VIBRATION_OFF_LEVEL);
    }
    return ESP_OK;
}

/**
 * 停止振动
 */
esp_err_t vibration_stop(void)
{
    gpio_set_level(COLLAR_MOTOR_GPIO, VIBRATION_OFF_LEVEL);
    return ESP_OK;
}
