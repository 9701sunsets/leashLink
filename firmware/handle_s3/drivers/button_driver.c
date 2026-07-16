#include "button_driver.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "button";

static const gpio_num_t k_button_gpios[] = {
    HANDLE_BUTTON_1_GPIO,
    HANDLE_BUTTON_2_GPIO,
    HANDLE_BUTTON_3_GPIO,
};

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

    ESP_LOGI(TAG, "ready: B1=GPIO%d B2=GPIO%d B3=GPIO%d active_low",
             HANDLE_BUTTON_1_GPIO, HANDLE_BUTTON_2_GPIO, HANDLE_BUTTON_3_GPIO);
    return ESP_OK;
}

bool button_driver_is_pressed(uint8_t index)
{
    if (index == 0 || index > 3) {
        return false;
    }
    return gpio_get_level(k_button_gpios[index - 1]) == 0;
}

ll_button_state_t button_driver_get_state(void)
{
    ll_button_state_t state = {
        .button1_pressed = button_driver_is_pressed(1),
        .button2_pressed = button_driver_is_pressed(2),
        .button3_pressed = button_driver_is_pressed(3),
    };
    return state;
}
