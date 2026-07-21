#include "leash_control.h"

#include "esp_log.h"

#include "servo_driver.h"

static const char *TAG = "leash_control";
static ll_leash_state_t s_state = LL_LEASH_UNLOCKED;

/**
 * 初始化牵引控制
 */
esp_err_t leash_control_init(void)
{
    ESP_ERROR_CHECK(servo_init());
    s_state = LL_LEASH_UNLOCKED;
    return servo_unlock();
}

/**
 * 锁定牵引
 * @param hold_ms 锁定保持时间（毫秒）
 */
esp_err_t leash_control_lock(uint32_t hold_ms)
{
    (void)hold_ms;
    if (s_state == LL_LEASH_LOCKED) {
        return ESP_OK;
    }
    ESP_LOGW(TAG, "lock leash");
    s_state = LL_LEASH_LOCKED;
    return servo_lock();
}

/**
 * 解锁牵引
 */
esp_err_t leash_control_unlock(void)
{
    if (s_state == LL_LEASH_UNLOCKED) {
        return ESP_OK;
    }
    ESP_LOGI(TAG, "unlock leash");
    s_state = LL_LEASH_UNLOCKED;
    return servo_unlock();
}

/**
 * 获取当前牵引状态
 */
ll_leash_state_t leash_control_get_state(void)
{
    return s_state;
}
