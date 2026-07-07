#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t leash_control_init(void);
esp_err_t leash_control_lock(uint32_t hold_ms);
esp_err_t leash_control_unlock(void);
ll_leash_state_t leash_control_get_state(void);

