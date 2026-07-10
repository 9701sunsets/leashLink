#include "servo_driver.h"

#include "driver/ledc.h"
#include "esp_log.h"

#include "board_pins.h"

static const char *TAG = "servo";

#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500
#define SERVO_PERIOD_US 20000
#define SERVO_DUTY_RES LEDC_TIMER_14_BIT

static uint32_t angle_to_duty(uint8_t angle)
{
    if (angle > 180) angle = 180;
    uint32_t pulse = SERVO_MIN_US + ((SERVO_MAX_US - SERVO_MIN_US) * angle) / 180;
    return (pulse * ((1 << SERVO_DUTY_RES) - 1)) / SERVO_PERIOD_US;
}

esp_err_t servo_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = HANDLE_SERVO_LEDC_MODE,
        .timer_num = HANDLE_SERVO_LEDC_TIMER,
        .duty_resolution = SERVO_DUTY_RES,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel = {
        .gpio_num = HANDLE_SERVO_PWM_GPIO,
        .speed_mode = HANDLE_SERVO_LEDC_MODE,
        .channel = HANDLE_SERVO_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = HANDLE_SERVO_LEDC_TIMER,
        .duty = angle_to_duty(90),
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
    ESP_LOGI(TAG, "initialized");
    return ESP_OK;
}

esp_err_t servo_set_angle(uint8_t angle_deg)
{
    ESP_ERROR_CHECK(ledc_set_duty(HANDLE_SERVO_LEDC_MODE, HANDLE_SERVO_LEDC_CH, angle_to_duty(angle_deg)));
    return ledc_update_duty(HANDLE_SERVO_LEDC_MODE, HANDLE_SERVO_LEDC_CH);
}

esp_err_t servo_lock(void)
{
    ESP_LOGW(TAG, "lock");
    return servo_set_angle(140);
}

esp_err_t servo_unlock(void)
{
    ESP_LOGI(TAG, "unlock");
    return servo_set_angle(50);
}

esp_err_t servo_stop(void)
{
    return ledc_stop(HANDLE_SERVO_LEDC_MODE, HANDLE_SERVO_LEDC_CH, 0);
}

