#pragma once

#include "esp_err.h"

esp_err_t led_init(void);
esp_err_t led_set_pattern(uint8_t pattern);
esp_err_t led_stop(void);
