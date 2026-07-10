#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t distance_service_init(void);
float distance_service_estimate_from_rssi(int8_t rssi_dbm);
bool distance_service_is_fence_breach(const ll_gps_fix_t *fix);

