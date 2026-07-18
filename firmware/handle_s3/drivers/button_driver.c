#include "button_driver.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "button";

static const gpio_num_t k_button_gpios[] = {
    HANDLE_BUTTON_1_GPIO,
    HANDLE_BUTTON_2_GPIO,
    HANDLE_BUTTON_3_GPIO,
};
static int s_idle_levels[3] = {1, 1, 1};

esp_err_t button_driver_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << HANDLE_BUTTON_1_GPIO) |
                        (1ULL << HANDLE_BUTTON_2_GPIO) |
                        (1ULL << HANDLE_BUTTON_3_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    vTaskDelay(pdMS_TO_TICKS(20));
    for (uint8_t i = 0; i < 3; ++i) {
        s_idle_levels[i] = gpio_get_level(k_button_gpios[i]);
    }

    ESP_LOGI(TAG, "ready: B1=GPIO%d idle=%d B2=GPIO%d idle=%d B3=GPIO%d idle=%d adaptive",
             HANDLE_BUTTON_1_GPIO, s_idle_levels[0],
             HANDLE_BUTTON_2_GPIO, s_idle_levels[1],
             HANDLE_BUTTON_3_GPIO, s_idle_levels[2]);
    return ESP_OK;
}

bool button_driver_is_pressed(uint8_t index)
{
    if (index == 0 || index > 3) {
        return false;
    }
    uint8_t idx = index - 1;
    return gpio_get_level(k_button_gpios[idx]) != s_idle_levels[idx];
}

ll_button_state_t button_driver_get_state(void)
{
    int l1 = gpio_get_level(HANDLE_BUTTON_1_GPIO);
    int l2 = gpio_get_level(HANDLE_BUTTON_2_GPIO);
    int l3 = gpio_get_level(HANDLE_BUTTON_3_GPIO);
    ll_button_state_t state = {
        .button1_pressed = l1 != s_idle_levels[0],
        .button2_pressed = l2 != s_idle_levels[1],
        .button3_pressed = l3 != s_idle_levels[2],
        .button1_level = l1,
        .button2_level = l2,
        .button3_level = l3,
    };
    return state;
}
