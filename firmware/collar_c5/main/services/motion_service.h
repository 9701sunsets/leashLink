#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t motion_service_init(void);
ll_motion_state_t motion_service_get_state(void);
void motion_service_update(uint16_t accel_peak_mg, uint8_t confidence_pct);
