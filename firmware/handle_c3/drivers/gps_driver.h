#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t gps_init(void);
esp_err_t gps_poll(ll_gps_fix_t *out);
ll_gps_fix_t gps_get_latest(void);

