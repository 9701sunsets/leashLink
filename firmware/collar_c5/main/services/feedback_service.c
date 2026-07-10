#include "feedback_service.h"

#include "esp_log.h"

#include "buzzer_driver.h"
#include "led_driver.h"
#include "motion_service.h"
#include "vibration_driver.h"

static const char *TAG = "feedback";

/**
 * 初始化反馈服务
 */
esp_err_t feedback_service_init(void)
{
    ESP_LOGI(TAG, "feedback service initialized");
    return ESP_OK;
}

/**
 * 处理反馈命令
 * @param cmd 控制命令
 * @return ESP_OK on success, or an error code on failure
 */
esp_err_t feedback_service_handle_cmd(const ll_control_cmd_t *cmd)
{
    if (!cmd) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (cmd->cmd_type) {
        case LL_FEEDBACK_STOP:
            led_stop();
            buzzer_stop();
            vibration_stop();
            break;
        case LL_FEEDBACK_WARNING:
            buzzer_beep(120);
            vibration_run(120);
            break;
        case LL_FEEDBACK_DANGER:
            buzzer_beep(300);
            vibration_run(300);
            led_set_pattern(2);
            break;
        case LL_FEEDBACK_FIND_DOG:
            led_set_pattern(1);
            break;
        case LL_FEEDBACK_SET_LED_PATTERN:
            led_set_pattern((uint8_t)cmd->param_a);
            break;
        case LL_FEEDBACK_ENTER_LOW_POWER:
            ESP_LOGI(TAG, "enter low power");
            break;
        default:
            ESP_LOGW(TAG, "unknown cmd=%u", cmd->cmd_type);
            break;
    }

    return ESP_OK;
}
