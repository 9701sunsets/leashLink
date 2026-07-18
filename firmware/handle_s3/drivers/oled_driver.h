#pragma once

#include "esp_err.h"
#include "leashlink_types.h"

esp_err_t oled_init(void);
esp_err_t oled_show_status(const ll_tension_sample_t *tension,
                           const ll_collar_telemetry_t *collar,
                           ll_safety_state_t safety_state,
                           int battery_pct);
esp_err_t oled_show_message(const char *line1, const char *line2);
esp_err_t oled_show_lines(const char *line0,
                          const char *line1,
                          const char *line2,
                          const char *line3);
