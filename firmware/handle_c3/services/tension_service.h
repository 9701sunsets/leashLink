#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t tension_service_init(void);
esp_err_t tension_service_sample(ll_tension_sample_t *out);
ll_tension_sample_t tension_service_get_latest(void);

