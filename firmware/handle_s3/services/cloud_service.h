#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t cloud_service_init(void);
esp_err_t cloud_service_publish_telemetry(const ll_tension_sample_t *tension,
                                          const ll_collar_telemetry_t *collar,
                                          ll_leash_state_t leash_state,
                                          int battery_pct);
esp_err_t cloud_service_publish_event(const ll_safety_event_t *event);

