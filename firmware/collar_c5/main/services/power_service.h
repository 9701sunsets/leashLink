#pragma once

#include "esp_err.h"

esp_err_t power_service_init(void);
uint8_t power_service_get_battery_pct(void);
