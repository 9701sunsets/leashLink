#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t safety_service_init(const ll_safety_config_t *config);
ll_safety_state_t safety_service_get_state(void);
esp_err_t safety_service_eval(const ll_safety_input_t *input, ll_safety_event_t *event_out);

