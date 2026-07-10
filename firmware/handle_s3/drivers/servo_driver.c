#include "servo_driver.h"

#include "driver/ledc.h"
#include "esp_log.h"

#include "board_pins.h"

static const char *TAG = "servo";

#define SERVO_MIN_US 500// 最小脉冲宽度（微秒）
#define SERVO_MAX_US 2500// 最大脉冲宽度（微秒）
#define SERVO_PERIOD_US 20000// 周期（微秒）
#define SERVO_DUTY_RES LEDC_TIMER_14_BIT// 占空比分辨率（位）

/**
 * 将角度转换为PWM占空比
 * @param angle 角度（0-180）
 * @return 占空比
 */
static uint32_t angle_to_duty(uint8_t angle)
{
    if (angle > 180) angle = 180;
    uint32_t pulse = SERVO_MIN_US + ((SERVO_MAX_US - SERVO_MIN_US) * angle) / 180;
    return (pulse * ((1 << SERVO_DUTY_RES) - 1)) / SERVO_PERIOD_US;
}

/**
 * 初始化舵机
 */
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

/**
 * 设置舵机角度
 * @param angle_deg 角度（0-180）
 */
esp_err_t servo_set_angle(uint8_t angle_deg)
{
    ESP_ERROR_CHECK(ledc_set_duty(HANDLE_SERVO_LEDC_MODE, HANDLE_SERVO_LEDC_CH, angle_to_duty(angle_deg)));
    return ledc_update_duty(HANDLE_SERVO_LEDC_MODE, HANDLE_SERVO_LEDC_CH);
}

/**
 * 锁定舵机
 */
esp_err_t servo_lock(void)
{
    ESP_LOGW(TAG, "lock");
    return servo_set_angle(140);
}

/**
 * 解锁舵机
 */
esp_err_t servo_unlock(void)
{
    ESP_LOGI(TAG, "unlock");
    return servo_set_angle(50);
}

/**
 * 停止PWM输出
 */
esp_err_t servo_stop(void)
{
    return ledc_stop(HANDLE_SERVO_LEDC_MODE, HANDLE_SERVO_LEDC_CH, 0);
}

