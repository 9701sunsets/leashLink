#pragma once

#include "esp_err.h"

esp_err_t power_init(void);
int power_get_battery_pct(void);
int power_get_light_lux_est(void);

