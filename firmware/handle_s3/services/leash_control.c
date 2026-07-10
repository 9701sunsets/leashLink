#include "leash_control.h"

#include "esp_log.h"

#include "servo_driver.h"

static const char *TAG = "leash_control";
static ll_leash_state_t s_state = LL_LEASH_UNLOCKED;

esp_err_t leash_control_init(void)
{
    ESP_ERROR_CHECK(servo_init());
    s_state = LL_LEASH_UNLOCKED;
    return servo_unlock();
}

esp_err_t leash_control_lock(uint32_t hold_ms)
{
    (void)hold_ms;
    ESP_LOGW(TAG, "lock leash");
    s_state = LL_LEASH_LOCKED;
    return servo_lock();
}

esp_err_t leash_control_unlock(void)
{
    ESP_LOGI(TAG, "unlock leash");
    s_state = LL_LEASH_UNLOCKED;
    return servo_unlock();
}

ll_leash_state_t leash_control_get_state(void)
{
    return s_state;
}

