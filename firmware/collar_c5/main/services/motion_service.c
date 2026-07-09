#include "motion_service.h"

#include "esp_log.h"

static const char *TAG = "motion";
static ll_motion_state_t s_state = LL_MOTION_IDLE;

esp_err_t motion_service_init(void)
{
    s_state = LL_MOTION_IDLE;
    ESP_LOGI(TAG, "motion service initialized");
    return ESP_OK;
}

ll_motion_state_t motion_service_get_state(void)
{
    return s_state;
}

void motion_service_update(uint16_t accel_peak_mg, uint8_t confidence_pct)
{
    if (accel_peak_mg > 1800) {
        s_state = LL_MOTION_BURST;
    } else if (accel_peak_mg > 700) {
        s_state = LL_MOTION_RUN;
    } else if (confidence_pct > 60) {
        s_state = LL_MOTION_WALK;
    } else {
        s_state = LL_MOTION_IDLE;
    }
    (void)confidence_pct;
}
